import styles from "./template.module.css";

// Re-mounts on every navigation, so each page eases in instead of snapping.
export default function Template({ children }: { children: React.ReactNode }) {
  return <div className={styles.pageIn}>{children}</div>;
}
