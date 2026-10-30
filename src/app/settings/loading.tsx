import { SettingsLoadingSkeleton } from "@/components/skeletons";

// Shown the instant the route is clicked, before the server has answered.
export default function Loading() {
  return <SettingsLoadingSkeleton />;
}
