// Read-only operational check: no index initialization, reconciliation or account data output.
import { MongoClient } from "mongodb";
import { accountingHealth } from "../src/lib/accounting-health";

const client = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017", { serverSelectionTimeoutMS: 5000 });
try {
  await client.connect();
  const report = await accountingHealth(client.db(process.env.MONGODB_DB ?? "baremetal"));
  console.log(JSON.stringify(report, null, 2));
  if (report.needsAttention) process.exitCode = 1;
} finally {
  await client.close();
}
