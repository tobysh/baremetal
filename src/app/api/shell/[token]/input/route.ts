import { shell } from "@/lib/shell/web-live";

export async function POST(req: Request, ctx: RouteContext<"/api/shell/[token]/input">) {
  return shell.input(req, (await ctx.params).token);
}
