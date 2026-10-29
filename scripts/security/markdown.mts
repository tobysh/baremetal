// Run without react-server conditions: node --import tsx scripts/security/markdown.mts
import assert from "node:assert/strict";
import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { Markdown } from "../../src/components/markdown";
import { MiniMarkdown } from "../../src/components/workspace/mini-markdown";
const attack = '<img src=x onerror="alert(1)">\n\n[click](javascript:alert%281%29)\n\n<script>alert(1)</script>\n\n**safe**';
for (const Component of [Markdown, MiniMarkdown]) {
  const html = Component === Markdown
    ? renderToStaticMarkup(createElement(Markdown, null, attack))
    : renderToStaticMarkup(createElement(MiniMarkdown, { text: attack }));
  assert(!/<script\b|<img\b|href="javascript:/i.test(html), html);
  assert(html.includes("<strong>safe</strong>"));
}
console.log("Both Markdown renderers reject executable markup and javascript links.");
