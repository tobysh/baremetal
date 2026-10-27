// Promote an existing account to admin: npm run make-admin -- you@example.com
// The account must then enable 2FA before /admin will let it in.
import { MongoClient } from "mongodb";

const email = process.argv[2]?.trim().toLowerCase();
if (!email) {
  console.error("Usage: npm run make-admin -- <email>");
  process.exit(1);
}
const client = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017");
await client.connect();
const res = await client.db(process.env.MONGODB_DB ?? "baremetal").collection("users").updateOne({ email }, { $set: { role: "admin" } });
console.log(res.matchedCount ? `${email} is now an admin. They must turn on 2FA at /settings/security to open /admin.` : `No account found for ${email}.`);
await client.close();
