import mongoose from "mongoose";

const connectMongoDB = async () => {
    try {
        await mongoose.connect(process.env.CONNECTION_MONGODB);
        console.log("Conectado con MONGODB");
    } catch (error) {
        console.log("Error al conectarse a MONGODB");
    }
}

export default connectMongoDB;