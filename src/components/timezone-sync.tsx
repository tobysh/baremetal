"use client";

import { useEffect } from "react";
import { rememberTimezone } from "@/app/settings/actions";

/** Tells the server this browser's time zone once, so the learner's weeks start on their own Monday. Renders nothing. */
export function TimezoneSync() {
  useEffect(() => {
    try {
      const tz = Intl.DateTimeFormat().resolvedOptions().timeZone;
      if (tz) void rememberTimezone(tz);
    } catch {}
  }, []);
  return null;
}
