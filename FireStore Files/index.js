const { https } = require('firebase-functions');  
const functions = require('firebase-functions');
const admin = require('firebase-admin');
const express = require('express');
const { Storage } = require("@google-cloud/storage");
const { stringify } = require("csv-stringify/sync");
const cors = require('cors')({ origin: true });

admin.initializeApp();
const db = admin.firestore();
const app = express();
app.use(express.json());

app.post('/addSamples', async (req, res) => {
    try {
        const {timestamp,sensorID,sensorModel,unitMac,samples } = req.body;

        const collectionName = `Sampling_${timestamp.replace(/[:\-TZ]/g, '_')}`;
        const collectionRef = db.collection(collectionName);

        const batch = db.batch();
        samples.forEach(({ sample, sample_index }) => {
            const docRef = collectionRef.doc();
            batch.set(docRef, {
                timestamp,
                sensorID,
                sensorModel,
                unitMac,
                sample,
                sample_index,
            });
        });

        await batch.commit();

        res.status(200).send({ message: 'Samples added successfully!' });
    } catch (error) {
        console.error('Error adding samples:', error);
        res.status(500).send({ message: 'Error adding samples.', error: error.message });
    }
});


const { onRequest } = require("firebase-functions/v2/https");
const storage = new Storage();
const bucket = admin.storage().bucket();


exports.exportAllToCSV = onRequest(async (req, res) => {
    try {
        const collections = await db.listCollections();
        const timestampCollections = collections.filter(col => col.id.startsWith("Sampling_"));

        if (timestampCollections.length === 0) {
            return res.status(404).send({ message: "No uploaded data found to export." });
        }

        for (const collection of timestampCollections) {
            const collectionName = collection.id;
            const fileName = `${collectionName}.csv`;
            const file = bucket.file(fileName);
            const writeStream = file.createWriteStream({ contentType: "text/csv" });

            writeStream.write("TIMESTAMP,SENSOR_ID,SENSOR_MODEL,UNIT_MAC,SAMPLE,SAMPLE_INDEX\n");

            const snapshot = await db.collection(collectionName).orderBy("sample_index", "asc").stream();

            for await (const doc of snapshot) {
                const data = doc.data();
                writeStream.write(`${data.timestamp},${data.sensorID},${data.sensorModel},${data.unitMac},${data.sample},${data.sample_index}\n`);
            }

            writeStream.end();

            console.log(`Exported ${collectionName} to ${fileName}`);
            await deleteCollection(collectionName);
            console.log(`Deleted collection ${collectionName}`);
        }

        res.status(200).send({ message: "Exported data to Cloud Storage successfully." });
    } catch (error) {
        console.error("Error exporting collections:", error);
        res.status(500).send({ error: "An error occurred while exporting data." });
    }
});


// Function to delete all documents in a collection in batches
async function deleteCollection(collectionPath) {
    const collectionRef = db.collection(collectionPath);
    let deleted = 0;

    while (true) {
        const snapshot = await collectionRef.limit(500).get();

        if (snapshot.empty) {
            console.log(`Deleted ${deleted} documents from ${collectionPath}`);
            break;
        }

        const batch = db.batch();
        snapshot.docs.forEach(doc => batch.delete(doc.ref));

        await batch.commit();
        deleted += snapshot.size;
    }

    console.log(`Collection ${collectionPath} deleted.`);
}

app.get('/exportAll', exports.exportAllToCSV);

exports.api = functions.https.onRequest(app);
