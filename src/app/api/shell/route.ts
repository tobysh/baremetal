import { shell } from "@/lib/shell/web-live";

export const POST = (req: Request) => shell.create(req);
