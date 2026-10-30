import { shell } from "@/lib/shell/web-live";

export async function GET(req: Request, ctx: RouteContext<"/api/shell/[token]">) {
  return shell.stream(req, (await ctx.params).token);
}

export async function DELETE(req: Request, ctx: RouteContext<"/api/shell/[token]">) {
  return shell.close(req, (await ctx.params).token);
}
