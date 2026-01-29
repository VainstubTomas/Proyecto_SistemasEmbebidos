import express from "express";
import http from "http";
import dotenv from "dotenv";
import connectMongoDB from "./dbConfig/db.js";
import Parking from "./models/Parking.js";
import ParkingEvent from "./models/ParkingEvent.js";

//variables de entorno
dotenv.config();

const app = express();
app.use(express.json());
app.use(express.static("public"));
const server = http.createServer(app);

//conexion bd
connectMongoDB();

app.post("/api/parking", async (req, res) => {
    try {
        const { lugaresLibres, ocupados, p1, p2 } = req.body;
        const registro = await Parking.create({ lugaresLibres, ocupados, p1, p2 });
        res.status(201).json(registro);
    } catch (error) {
        res.status(500).json({ error: "Error al guardar datos" });
    }
});

app.post("/api/parking/event", async (req, res) => {
    try {
        const { plaza, duracionMs } = req.body;
        const endedAt = new Date();
        const startedAt = new Date(endedAt.getTime() - Number(duracionMs || 0));
        const evento = await ParkingEvent.create({ plaza, duracionMs, startedAt, endedAt });
        res.status(201).json(evento);
    } catch (error) {
        res.status(500).json({ error: "Error al guardar evento" });
    }
});

server.listen(8080, ()=>{
    console.log("Servidor iniciado correctamente en el puerto 8080");
});