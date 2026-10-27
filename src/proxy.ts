import { NextResponse, type NextRequest } from "next/server";

// Optimistic check only: sends signed-out visitors straight to the login page
// instead of rendering a protected page shell first. The real checks (valid
// session, admin role, 2FA) run on the server in requireUser/requireAdmin,
// and in every route handler.
export function proxy(req: NextRequest) {
  if (!req.cookies.has("bm_session")) {
    const url = new URL("/login", req.url);
    url.searchParams.set("next", req.nextUrl.pathname);
    return NextResponse.redirect(url);
  }
  return NextResponse.next();
}

export const config = {
  matcher: ["/dashboard/:path*", "/settings/:path*", "/admin/:path*"],
};
