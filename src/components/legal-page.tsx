import Link from "next/link";
import { getLegalDoc, type LegalDoc } from "@/lib/legal";
import { Markdown } from "./markdown";
import { Container } from "./ui";

const DOCS: { id: LegalDoc; title: string }[] = [
  { id: "terms", title: "Terms of Service" },
  { id: "privacy", title: "Privacy Notice" },
  { id: "cookies", title: "Cookie Policy" },
];

export function LegalPage({ doc, title }: { doc: LegalDoc; title: string }) {
  return (
    <Container className="max-w-3xl py-12">
      <nav aria-label="Legal documents" className="flex flex-wrap gap-2 text-sm">
        {DOCS.map((d) => (
          <Link
            key={d.id}
            href={`/${d.id}`}
            aria-current={d.id === doc ? "page" : undefined}
            className={`rounded-full border px-3 py-1 ${d.id === doc ? "border-accent bg-accent-soft text-text" : "border-border text-muted hover:text-text"}`}
          >
            {d.title}
          </Link>
        ))}
      </nav>
      <h1 className="mt-8 text-3xl font-bold tracking-tight sm:text-4xl">{title}</h1>
      <Markdown className="mt-6">{getLegalDoc(doc)}</Markdown>
    </Container>
  );
}
