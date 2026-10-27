import { getLesson, getTracks } from "@/lib/curriculum";
import { OG_SIZE, ogImage } from "@/lib/og";

export const size = OG_SIZE;
export const contentType = "image/png";
export const alt = "Baremetal lesson";

export function generateStaticParams() {
  return getTracks().flatMap((t) => t.lessons.map((l) => ({ track: t.slug, lesson: l.slug })));
}

export default async function Image({ params }: { params: Promise<{ track: string; lesson: string }> }) {
  const { track: t, lesson: l } = await params;
  const found = getLesson(t, l);
  return ogImage({
    eyebrow: found ? `${found.track.title} · Lesson ${found.index + 1}` : "Lesson",
    title: found?.lesson.title ?? "Baremetal",
    subtitle: found?.lesson.summary ?? "",
  });
}
