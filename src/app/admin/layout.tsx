import { Suspense } from "react";
import { connection } from "next/server";
import { Container, Skeleton } from "@/components/ui";
import { getSystemHealth } from "@/lib/admin";
import { failingChecks } from "@/lib/admin-health";
import { adminFromRequest } from "@/lib/auth";
import { AdminNav } from "./admin-nav";

async function NavWithHealth() {
  await connection();
  let failing = 0;
  // Only admins trigger the health query; the pages do their own authorization.
  if (await adminFromRequest()) {
    try {
      failing = failingChecks(await getSystemHealth()).length;
    } catch {
      failing = 1;
    }
  }
  return <AdminNav healthFailing={failing} />;
}

export default function AdminLayout({ children }: { children: React.ReactNode }) {
  return (
    <>
      <Container className="pt-8">
        <Suspense fallback={<Skeleton className="h-10" />}>
          <NavWithHealth />
        </Suspense>
      </Container>
      {children}
    </>
  );
}
