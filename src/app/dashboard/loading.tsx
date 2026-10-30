import { Container } from "@/components/ui";
import { DashboardSkeleton } from "@/components/skeletons";

// Shown the instant /dashboard is clicked, before the server has answered.
export default function Loading() {
  return <Container className="py-10 sm:py-12"><DashboardSkeleton /></Container>;
}
