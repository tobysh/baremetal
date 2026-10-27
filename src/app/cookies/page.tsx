import type { Metadata } from "next";
import { LegalPage } from "@/components/legal-page";

export const metadata: Metadata = {
  title: "Cookie Policy",
  description: "The cookies Baremetal uses, why each one is needed, how long it lasts, and how to change your choices at any time.",
  alternates: { canonical: "/cookies" },
};

export default function Page() {
  return <LegalPage doc="cookies" title="Cookie Policy" />;
}
