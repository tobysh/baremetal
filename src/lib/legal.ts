import "server-only";
import { readFileSync } from "node:fs";
import path from "node:path";
import { SITE } from "./site";

export type LegalDoc = "privacy" | "terms" | "cookies";

/** Loads content/legal/<doc>.md and fills in the operator's details. */
export function getLegalDoc(doc: LegalDoc): string {
  const raw = readFileSync(path.join(process.cwd(), "content", "legal", `${doc}.md`), "utf8");
  const vars: Record<string, string> = {
    entity: SITE.legal.entity,
    address: SITE.legal.address,
    contactEmail: SITE.legal.contactEmail,
    jurisdiction: SITE.legal.jurisdiction,
    lastUpdated: SITE.legal.lastUpdated,
    siteUrl: SITE.url,
    ...tutorText(),
  };
  return raw.replace(/\{\{(\w+)\}\}/g, (m, key: string) => vars[key] ?? m);
}

/** Tutor wording depends on whether Claude (Anthropic) or a self-hosted Ollama model is configured. */
function tutorText() {
  if (process.env.TUTOR_PROVIDER === "ollama") {
    return {
      tutorShort: "Your AI tutor chats are answered by an AI model running on our own server; they are not sent to an AI company.",
      tutorProcessing:
        "The tutor runs on an AI model hosted on our own infrastructure, so this information is not sent to any third-party AI provider.",
      tutorSharing: "",
    };
  }
  return {
    tutorShort: "Your AI tutor chats are sent to Anthropic so the tutor can reply.",
    tutorProcessing:
      "This information is sent to Anthropic, PBC, which makes the Claude Haiku 5.5 AI model that writes the reply. Anthropic acts as our processor, which means it handles this data for us. Under its commercial terms, inputs and outputs sent through the API are not used to train its models.\n\nYour data may be transferred to the United States. We use Standard Contractual Clauses and the UK International Data Transfer Addendum to protect it when it moves.",
    tutorSharing: "- **Anthropic, PBC**: receives AI tutor requests as described above.\n",
  };
}
