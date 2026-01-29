import mongoose from "mongoose";

const parkingSchema = new mongoose.Schema(
  {
    lugaresLibres: { type: Number, required: true },
    ocupados: { type: Number, required: true },
    p1: { type: Number, required: true },
    p2: { type: Number, required: true },
  },
  { timestamps: true }
);

export default mongoose.model("Parking", parkingSchema);
