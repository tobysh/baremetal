import { OG_SIZE, ogImage } from "@/lib/og";

export const size = OG_SIZE;
export const contentType = "image/png";
export const alt = "Baremetal — learn how computers really work by writing C, Rust, Go and assembly";

export default function Image() {
  return ogImage({
    eyebrow: "Learn low-level programming from zero",
    title: "Learn how computers really work.",
    subtitle: "From reversing strings in C to building your own HTTP server in Rust, with an AI tutor that guides instead of giving answers.",
  });
}
