import "server-only";
import nodemailer from "nodemailer";

export async function sendEmail(to: string, subject: string, text: string) {
  const smtp = process.env.SMTP_URL;
  if (!smtp) {
    if (process.env.NODE_ENV === "production") throw new Error("SMTP_URL is not configured");
    // Development fallback: print the email so the flow can be tested locally.
    console.info(`\n[email:dev] To: ${to}\nSubject: ${subject}\n\n${text}\n`);
    return;
  }
  const transport = nodemailer.createTransport(smtp);
  await transport.sendMail({ from: process.env.EMAIL_FROM ?? "no-reply@example.com", to, subject, text });
}
