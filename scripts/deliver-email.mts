// Run periodically (for example every minute) to retry queued email after restarts:
// npx tsx --conditions=react-server --env-file=.env.local scripts/deliver-email.mts
import { drainEmailQueue } from "../src/lib/email";
try {
  await drainEmailQueue();
  process.exit(0);
} catch (error) {
  console.error("Email queue worker failed", error);
  process.exit(1);
}
