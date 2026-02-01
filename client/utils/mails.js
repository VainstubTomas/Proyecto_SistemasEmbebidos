import nodemailer from "nodemailer";

//transporte
let Transporter = nodemailer.createTransport({
    service: 'gmail',
    auth: {
        user: 'vainstubtomas@gmail.com',
        pass: 'vyon fqiv jqtj cwsy'
    }
});

/**
 * Envía un correo electrónico de notificación de pago
 * @param {number} monto - Monto del pago
 * @param {string} espacio - Espacio de parking
 * @param {number} duracionMinutos - Duración en minutos
 * @returns {Promise<Object>} - Información del envío
 */
export async function enviarNotificacionPago(monto, espacio, duracionMinutos) {
    const mailOptions = {
        from: 'vainstubtomas@gmail.com', // CORREGIDO: era 'for'
        to: 'vainstubtomas@gmail.com',
        subject: 'Pago parking',
        text: `Usted ha recibido $${monto} pesos por el uso del espacio ${espacio} durante ${duracionMinutos.toFixed(2)} minutos.`
    };

    try {
        const info = await Transporter.sendMail(mailOptions);
        console.log("[MAILS] Correo enviado con éxito:", info.messageId);
        return { success: true, info };
    } catch (error) {
        console.error("[MAILS] Error al enviar correo:", error.message);
        return { success: false, error: error.message };
    }
}

export default Transporter;