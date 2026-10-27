import type { Metadata } from "next";
import { LegalPage } from "@/components/legal-page";

export const metadata: Metadata = {
  title: "Privacy Notice",
  description: "How Baremetal collects, uses and protects your personal data, your rights under UK and EU GDPR, and how to exercise them.",
  alternates: { canonical: "/privacy" },
};

export default function Page() {
  return <LegalPage doc="privacy" title="Privacy Notice" />;
}
