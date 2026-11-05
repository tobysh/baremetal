import { Suspense } from "react";
import { Container, Skeleton } from "@/components/ui";
import { AdminNav } from "./admin-nav";

export default function AdminLayout({ children }: { children: React.ReactNode }) {
  return (
    <>
      <Container className="pt-8">
        <Suspense fallback={<Skeleton className="h-10" />}>
          <AdminNav />
        </Suspense>
      </Container>
      {children}
    </>
  );
}
