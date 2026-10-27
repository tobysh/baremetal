import ReactMarkdown from "react-markdown";
import remarkGfm from "remark-gfm";

// Server component: markdown is rendered to HTML on the server, so neither the
// parser nor the content ships to the browser as JavaScript.
export function Markdown({ children, className = "" }: { children: string; className?: string }) {
  return (
    <div className={`prose ${className}`}>
      <ReactMarkdown
        remarkPlugins={[remarkGfm]}
        components={{
          // Page templates own the single <h1>; demote any stray one in content.
          h1: ({ children }) => <h2>{children}</h2>,
          a: ({ href, children }) => {
            const external = href?.startsWith("http");
            return (
              <a href={href} {...(external ? { target: "_blank", rel: "noopener noreferrer" } : {})}>
                {children}
              </a>
            );
          },
          pre: ({ children }) => <pre tabIndex={0}>{children}</pre>,
        }}
      >
        {children}
      </ReactMarkdown>
    </div>
  );
}
