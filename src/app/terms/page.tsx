import type { Metadata } from "next";
import { LegalPage } from "@/components/legal-page";

export const metadata: Metadata = {
  title: "Terms of Service",
  description: "The rules for using Baremetal: your account, acceptable use of the code runner and AI tutor, your content, and our responsibilities.",
  alternates: { canonical: "/terms" },
};

export default function Page() {
  return <LegalPage doc="terms" title="Terms of Service" />;
}
