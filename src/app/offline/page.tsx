import type { Metadata } from "next";
import { Container } from "@/components/ui";
import { OfflineActions } from "./actions";

export const metadata: Metadata = {
  title: "You're offline",
  description: "Baremetal needs a connection to load lessons and run code.",
  alternates: { canonical: "/offline" },
  robots: { index: false },
};

export default function OfflinePage() {
  return (
    <Container width="narrow" className="py-16">
      <h1 className="display text-3xl font-bold">You&apos;re offline</h1>
      <p className="mt-3 text-text-2">Lessons, running your code, tests and the tutor all need a connection to the server. Your saved progress is safe and will be here when you reconnect.</p>
      <OfflineActions />
    </Container>
  );
}
