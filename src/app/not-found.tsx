import type { Metadata } from "next";
import Link from "next/link";
import { ButtonLink, Container } from "@/components/ui";

export const metadata: Metadata = {
  title: "Page not found",
  description: "We couldn't find that page. Head back to the courses to keep learning C and Rust.",
  robots: { index: false },
};

export default function NotFound() {
  return (
    <Container className="max-w-2xl py-20 text-center">
      <p className="font-mono text-sm text-accent">error 404: segmentation fault (page not found)</p>
      <h1 className="mt-4 text-4xl font-bold tracking-tight">This page doesn&apos;t exist</h1>
      <p className="mt-4 text-lg text-muted">
        Like dereferencing a pointer to memory you don&apos;t own, this address points nowhere. It may have moved, or the link might have a typo.
      </p>
      <div className="mt-8 flex flex-wrap justify-center gap-3">
        <ButtonLink href="/learn" size="lg">
          Browse courses
        </ButtonLink>
        <ButtonLink href="/" size="lg" variant="secondary">
          Go home
        </ButtonLink>
      </div>
      <p className="mt-10 text-sm text-muted">
        Looking for somewhere to start?{" "}
        <Link href="/learn/first-steps/hello-world" className="text-accent underline underline-offset-2">
          Write your first program
        </Link>
        .
      </p>
    </Container>
  );
}
