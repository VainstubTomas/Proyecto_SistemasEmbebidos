import mongoose from "mongoose";

const parkingEventSchema = new mongoose.Schema(
  {
    plaza: { type: String, required: true },
    duracionMs: { type: Number, required: true },
    startedAt: { type: Date, required: true },
    endedAt: { type: Date, required: true },
  },
  { timestamps: true }
);

export default mongoose.model("ParkingEvent", parkingEventSchema);
