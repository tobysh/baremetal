export function LogoMark({ className = "h-7 w-7" }: { className?: string }) {
  return (
    <svg viewBox="0 0 32 32" className={className} aria-hidden="true" focusable="false">
      <rect width="32" height="32" rx="7" fill="var(--accent)" />
      <path d="M9 10.5 14.5 16 9 21.5" fill="none" stroke="var(--accent-ink)" strokeWidth="2.8" strokeLinecap="round" strokeLinejoin="round" />
      <path d="M17 22h7" stroke="var(--accent-ink)" strokeWidth="2.8" strokeLinecap="round" />
    </svg>
  );
}
