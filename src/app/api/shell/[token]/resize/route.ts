import { shell } from "@/lib/shell/web-live";

export async function POST(req: Request, ctx: RouteContext<"/api/shell/[token]/resize">) {
  return shell.resize(req, (await ctx.params).token);
}
