import { Fragment, type ReactNode } from "react";

// A tiny markdown renderer for tutor replies: paragraphs, lists, fenced code,
// inline code, bold and italics. Builds React elements (never raw HTML), so
// model output can't inject markup, and it keeps a full markdown library out
// of the browser bundle.

function inline(text: string, key: string): ReactNode[] {
  const out: ReactNode[] = [];
  const re = /(`[^`]+`|\*\*[^*]+\*\*|\*[^*\s][^*]*\*)/g;
  let last = 0;
  let i = 0;
  for (const m of text.matchAll(re)) {
    if (m.index! > last) out.push(text.slice(last, m.index));
    const tok = m[0];
    const k = `${key}-${i++}`;
    if (tok.startsWith("`")) out.push(<code key={k}>{tok.slice(1, -1)}</code>);
    else if (tok.startsWith("**")) out.push(<strong key={k}>{tok.slice(2, -2)}</strong>);
    else out.push(<em key={k}>{tok.slice(1, -1)}</em>);
    last = m.index! + tok.length;
  }
  if (last < text.length) out.push(text.slice(last));
  return out;
}

export function MiniMarkdown({ text }: { text: string }) {
  const blocks: ReactNode[] = [];
  const lines = text.split("\n");
  let i = 0;
  let n = 0;
  while (i < lines.length) {
    const line = lines[i];
    if (line.startsWith("```")) {
      const code: string[] = [];
      i++;
      while (i < lines.length && !lines[i].startsWith("```")) code.push(lines[i++]);
      i++;
      blocks.push(
        <pre key={n++} tabIndex={0}>
          <code>{code.join("\n")}</code>
        </pre>,
      );
    } else if (/^\s*([-*]|\d+\.)\s+/.test(line)) {
      const ordered = /^\s*\d+\./.test(line);
      const items: string[] = [];
      while (i < lines.length && /^\s*([-*]|\d+\.)\s+/.test(lines[i])) items.push(lines[i++].replace(/^\s*([-*]|\d+\.)\s+/, ""));
      const List = ordered ? "ol" : "ul";
      blocks.push(
        <List key={n++}>
          {items.map((it, j) => (
            <li key={j}>{inline(it, `${n}-${j}`)}</li>
          ))}
        </List>,
      );
    } else if (/^#{1,6}\s/.test(line)) {
      blocks.push(
        <p key={n++}>
          <strong>{inline(line.replace(/^#+\s/, ""), `h${n}`)}</strong>
        </p>,
      );
      i++;
    } else if (line.trim() === "") {
      i++;
    } else {
      const para: string[] = [];
      while (i < lines.length && lines[i].trim() !== "" && !lines[i].startsWith("```") && !/^\s*([-*]|\d+\.)\s+/.test(lines[i])) para.push(lines[i++]);
      blocks.push(
        <p key={n++}>
          {para.map((p, j) => (
            <Fragment key={j}>
              {j > 0 && <br />}
              {inline(p, `${n}-${j}`)}
            </Fragment>
          ))}
        </p>,
      );
    }
  }
  return <div className="prose text-sm">{blocks}</div>;
}
