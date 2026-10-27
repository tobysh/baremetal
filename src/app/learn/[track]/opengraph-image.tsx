import { getTrack, getTracks } from "@/lib/curriculum";
import { OG_SIZE, ogImage } from "@/lib/og";

export const size = OG_SIZE;
export const contentType = "image/png";
export const alt = "Baremetal course";

export function generateStaticParams() {
  return getTracks().map((t) => ({ track: t.slug }));
}

export default async function Image({ params }: { params: Promise<{ track: string }> }) {
  const track = getTrack((await params).track);
  return ogImage({
    eyebrow: track ? `${track.level} course · ${track.lessons.length} lessons · ${track.language === "c" ? "C" : "Rust"}` : "Course",
    title: track?.title ?? "Baremetal",
    subtitle: track?.summary ?? "",
  });
}
