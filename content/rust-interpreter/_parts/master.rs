use std::cell::RefCell;
use std::collections::HashMap;
use std::fmt;
use std::rc::Rc;

// ================= Tokens and the lexer =================

#[derive(Debug, PartialEq, Clone)]
enum Token {
    Num(i64),
    Ident(String),
    // keywords
    Let,
    If,
    Else,
    While,
    Fn,
    Return,
    Print,
    True,
    False,
    // operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Bang,
    Eq,     // =
    EqEq,   // ==
    BangEq, // !=
    Lt,
    LtEq,
    Gt,
    GtEq,
    // punctuation
    LParen,
    RParen,
    LBrace,
    RBrace,
    Comma,
    Semi,
}

/// A token and the byte offset where it starts in the source.
#[derive(Debug, PartialEq, Clone)]
struct Spanned {
    tok: Token,
    pos: usize,
}

#[derive(Debug, PartialEq, Clone)]
enum LexError {
    UnexpectedChar(char, usize),
    NumberTooBig(usize),
}

//@task 1
fn lex(src: &str) -> Result<Vec<Spanned>, LexError> {
//@stub
    // Your code here
    todo!()
//@sol
    let chars: Vec<(usize, char)> = src.char_indices().collect();
    let mut out = Vec::new();
    let mut i = 0;
    while i < chars.len() {
        let (pos, c) = chars[i];
        if c.is_whitespace() {
            i += 1;
            continue;
        }
        if c == '#' {
            while i < chars.len() && chars[i].1 != '\n' {
                i += 1;
            }
            continue;
        }
        if c.is_ascii_digit() {
            let start = i;
            while i < chars.len() && chars[i].1.is_ascii_digit() {
                i += 1;
            }
            let text: String = chars[start..i].iter().map(|&(_, d)| d).collect();
            let n = text.parse::<i64>().map_err(|_| LexError::NumberTooBig(pos))?;
            out.push(Spanned { tok: Token::Num(n), pos });
            continue;
        }
        if c.is_ascii_alphabetic() || c == '_' {
            let start = i;
            while i < chars.len() && (chars[i].1.is_ascii_alphanumeric() || chars[i].1 == '_') {
                i += 1;
            }
            let word: String = chars[start..i].iter().map(|&(_, d)| d).collect();
            let tok = match word.as_str() {
                "let" => Token::Let,
                "if" => Token::If,
                "else" => Token::Else,
                "while" => Token::While,
                "fn" => Token::Fn,
                "return" => Token::Return,
                "print" => Token::Print,
                "true" => Token::True,
                "false" => Token::False,
                _ => Token::Ident(word),
            };
            out.push(Spanned { tok, pos });
            continue;
        }
        let next = chars.get(i + 1).map(|&(_, n)| n);
        let (tok, len) = match (c, next) {
            ('=', Some('=')) => (Token::EqEq, 2),
            ('!', Some('=')) => (Token::BangEq, 2),
            ('<', Some('=')) => (Token::LtEq, 2),
            ('>', Some('=')) => (Token::GtEq, 2),
            ('=', _) => (Token::Eq, 1),
            ('!', _) => (Token::Bang, 1),
            ('<', _) => (Token::Lt, 1),
            ('>', _) => (Token::Gt, 1),
            ('+', _) => (Token::Plus, 1),
            ('-', _) => (Token::Minus, 1),
            ('*', _) => (Token::Star, 1),
            ('/', _) => (Token::Slash, 1),
            ('%', _) => (Token::Percent, 1),
            ('(', _) => (Token::LParen, 1),
            (')', _) => (Token::RParen, 1),
            ('{', _) => (Token::LBrace, 1),
            ('}', _) => (Token::RBrace, 1),
            (',', _) => (Token::Comma, 1),
            (';', _) => (Token::Semi, 1),
            _ => return Err(LexError::UnexpectedChar(c, pos)),
        };
        out.push(Spanned { tok, pos });
        i += len;
    }
    Ok(out)
//@foot
}
//@endtask

//@if >=2
// ================= The token stream =================

#[derive(Debug, PartialEq, Clone)]
struct ParseError {
    msg: String,
    pos: usize,
}

impl ParseError {
    fn new(msg: impl Into<String>, pos: usize) -> ParseError {
        ParseError { msg: msg.into(), pos }
    }
}

impl From<LexError> for ParseError {
    fn from(e: LexError) -> ParseError {
        match e {
            LexError::UnexpectedChar(c, pos) => ParseError::new(format!("unexpected character '{}'", c), pos),
            LexError::NumberTooBig(pos) => ParseError::new("number too big", pos),
        }
    }
}

/// The tokens of one source text plus a cursor. `end` is the source length, used as the
/// position of "end of input".
struct Stream {
    toks: Vec<Spanned>,
    i: usize,
    end: usize,
}

impl Stream {
    fn new(toks: Vec<Spanned>, end: usize) -> Stream {
        Stream { toks, i: 0, end }
    }

    fn from_source(src: &str) -> Result<Stream, ParseError> {
        Ok(Stream::new(lex(src)?, src.len()))
    }

//@task 2
    /// The next token, without consuming it.
    fn peek(&self) -> Option<&Token> {
//@stub
        todo!()
//@sol
        self.toks.get(self.i).map(|s| &s.tok)
//@foot
    }
//@endtask

//@task 2
    /// The token `n` places ahead (`peek_at(0)` is `peek()`).
    fn peek_at(&self, n: usize) -> Option<&Token> {
//@stub
        todo!()
//@sol
        self.toks.get(self.i + n).map(|s| &s.tok)
//@foot
    }
//@endtask

//@task 2
    /// Byte offset of the next token, or the end of the source when there are no more.
    fn pos(&self) -> usize {
//@stub
        todo!()
//@sol
        self.toks.get(self.i).map_or(self.end, |s| s.pos)
//@foot
    }
//@endtask

//@task 2
    /// Consume and return the next token.
    fn next(&mut self) -> Option<Spanned> {
//@stub
        todo!()
//@sol
        let t = self.toks.get(self.i).cloned();
        if t.is_some() {
            self.i += 1;
        }
        t
//@foot
    }
//@endtask

//@task 2
    /// Consume the next token only if it equals `t`. Says whether it did.
    fn eat(&mut self, t: &Token) -> bool {
//@stub
        todo!()
//@sol
        if self.peek() == Some(t) {
            self.i += 1;
            true
        } else {
            false
        }
//@foot
    }
//@endtask

//@task 2
    /// Consume `t` or fail. On success returns the token's position.
    fn expect(&mut self, t: &Token) -> Result<usize, ParseError> {
//@stub
        todo!()
//@sol
        let pos = self.pos();
        match self.peek() {
            Some(found) if found == t => {
                self.i += 1;
                Ok(pos)
            }
            Some(found) => Err(ParseError::new(format!("expected {:?}, found {:?}", t, found), pos)),
            None => Err(ParseError::new(format!("expected {:?}, found end of input", t), pos)),
        }
//@foot
    }
//@endtask

//@task 2
    /// Consume an identifier and return its name.
    fn expect_ident(&mut self) -> Result<String, ParseError> {
//@stub
        todo!()
//@sol
        let pos = self.pos();
        match self.peek().cloned() {
            Some(Token::Ident(name)) => {
                self.i += 1;
                Ok(name)
            }
            Some(found) => Err(ParseError::new(format!("expected identifier, found {:?}", found), pos)),
            None => Err(ParseError::new("expected identifier, found end of input", pos)),
        }
//@foot
    }
//@endtask

//@task 2
    fn at_end(&self) -> bool {
//@stub
        todo!()
//@sol
        self.i >= self.toks.len()
//@foot
    }
//@endtask
}
//@endif

//@if >=3
// ================= The syntax tree and the expression parser =================

#[derive(Debug, PartialEq, Clone, Copy)]
enum BinOp {
    Add,
    Sub,
    Mul,
    Div,
    Rem,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
}

impl BinOp {
    fn symbol(self) -> &'static str {
        match self {
            BinOp::Add => "+",
            BinOp::Sub => "-",
            BinOp::Mul => "*",
            BinOp::Div => "/",
            BinOp::Rem => "%",
            BinOp::Eq => "==",
            BinOp::Ne => "!=",
            BinOp::Lt => "<",
            BinOp::Le => "<=",
            BinOp::Gt => ">",
            BinOp::Ge => ">=",
        }
    }
}

#[derive(Debug, PartialEq, Clone, Copy)]
enum UnOp {
    Neg,
    Not,
}

#[derive(Debug, PartialEq, Clone)]
enum Expr {
    Num(i64),
    Bool(bool),
    Var(String),
    Unary(UnOp, Box<Expr>),
    Binary(BinOp, Box<Expr>, Box<Expr>),
    Call(Box<Expr>, Vec<Expr>),
}

/// S-expression text of a tree: `1 + 2 * 3` shows as `(+ 1 (* 2 3))`.
fn show(e: &Expr) -> String {
    match e {
        Expr::Num(n) => n.to_string(),
        Expr::Bool(b) => b.to_string(),
        Expr::Var(v) => v.clone(),
        Expr::Unary(op, x) => format!("({} {})", if *op == UnOp::Neg { "-" } else { "!" }, show(x)),
        Expr::Binary(op, l, r) => format!("({} {} {})", op.symbol(), show(l), show(r)),
        Expr::Call(f, args) => {
            let mut s = format!("(call {}", show(f));
            for a in args {
                s.push(' ');
                s.push_str(&show(a));
            }
            s.push(')');
            s
        }
    }
}

/// Binding powers. Higher binds tighter.
const PREFIX_BP: u8 = 7; // unary - and !
const CALL_BP: u8 = 9; // f(...)

/// If `t` is a binary operator: (left power, right power, operator).
/// Left power below right power makes the operator left-associative.
fn infix(t: &Token) -> Option<(u8, u8, BinOp)> {
    Some(match t {
        Token::EqEq => (1, 2, BinOp::Eq),
        Token::BangEq => (1, 2, BinOp::Ne),
        Token::Lt => (1, 2, BinOp::Lt),
        Token::LtEq => (1, 2, BinOp::Le),
        Token::Gt => (1, 2, BinOp::Gt),
        Token::GtEq => (1, 2, BinOp::Ge),
        Token::Plus => (3, 4, BinOp::Add),
        Token::Minus => (3, 4, BinOp::Sub),
        Token::Star => (5, 6, BinOp::Mul),
        Token::Slash => (5, 6, BinOp::Div),
        Token::Percent => (5, 6, BinOp::Rem),
        _ => return None,
    })
}

//@task 3
/// Parse one expression, stopping before any operator that binds more loosely than `min_bp`.
fn parse_expr(s: &mut Stream, min_bp: u8) -> Result<Expr, ParseError> {
//@stub
    todo!()
//@sol
    let first = match s.next() {
        Some(t) => t,
        None => return Err(ParseError::new("unexpected end of input", s.pos())),
    };
    let mut lhs = match first.tok {
        Token::Num(n) => Expr::Num(n),
        Token::True => Expr::Bool(true),
        Token::False => Expr::Bool(false),
        Token::Ident(name) => Expr::Var(name),
        Token::Minus => Expr::Unary(UnOp::Neg, Box::new(parse_expr(s, PREFIX_BP)?)),
        Token::Bang => Expr::Unary(UnOp::Not, Box::new(parse_expr(s, PREFIX_BP)?)),
        Token::LParen => {
            let inner = parse_expr(s, 0)?;
            s.expect(&Token::RParen)?;
            inner
        }
        other => return Err(ParseError::new(format!("unexpected {:?}", other), first.pos)),
    };
    loop {
        if s.peek() == Some(&Token::LParen) {
            if CALL_BP < min_bp {
                break;
            }
            s.next();
            let mut args = Vec::new();
            if !s.eat(&Token::RParen) {
                loop {
                    args.push(parse_expr(s, 0)?);
                    if s.eat(&Token::Comma) {
                        continue;
                    }
                    s.expect(&Token::RParen)?;
                    break;
                }
            }
            lhs = Expr::Call(Box::new(lhs), args);
            continue;
        }
        let (l_bp, r_bp, op) = match s.peek().and_then(infix) {
            Some(x) => x,
            None => break,
        };
        if l_bp < min_bp {
            break;
        }
        s.next();
        let rhs = parse_expr(s, r_bp)?;
        lhs = Expr::Binary(op, Box::new(lhs), Box::new(rhs));
    }
    Ok(lhs)
//@foot
}
//@endtask

/// Parse a whole source text as one expression.
fn parse_expression(src: &str) -> Result<Expr, ParseError> {
    let mut s = Stream::from_source(src)?;
    let e = parse_expr(&mut s, 0)?;
    match s.next() {
        None => Ok(e),
        Some(t) => Err(ParseError::new(format!("unexpected {:?}", t.tok), t.pos)),
    }
}
//@endif

//@if >=4
// ================= Values and evaluation =================

//@if <9
#[derive(Debug, PartialEq, Clone)]
enum Value {
    Int(i64),
    Bool(bool),
}
//@else
#[derive(Debug, PartialEq, Clone)]
enum Value {
    Int(i64),
    Bool(bool),
    Nil,
    Func(Rc<Function>),
}
//@endif

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Value::Int(n) => write!(f, "{}", n),
            Value::Bool(b) => write!(f, "{}", b),
//@if >=9
            Value::Nil => write!(f, "nil"),
            Value::Func(func) => write!(f, "<fn {}>", func.name),
//@endif
        }
    }
}

#[derive(Debug, PartialEq, Clone)]
enum RuntimeError {
    DivByZero,
    Overflow,
    TypeMismatch(String),
    Undefined(String),
    NotCallable,
    Arity { expected: usize, got: usize },
}

impl fmt::Display for RuntimeError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            RuntimeError::DivByZero => write!(f, "division by zero"),
            RuntimeError::Overflow => write!(f, "integer overflow"),
            RuntimeError::TypeMismatch(m) => write!(f, "type error: {}", m),
            RuntimeError::Undefined(n) => write!(f, "undefined variable '{}'", n),
            RuntimeError::NotCallable => write!(f, "value is not callable"),
            RuntimeError::Arity { expected, got } => write!(f, "expected {} arguments, got {}", expected, got),
        }
    }
}

//@task 4
/// `-` on an integer (overflow is an error), `!` on a boolean. Anything else is a type error.
fn apply_unary(op: UnOp, v: &Value) -> Result<Value, RuntimeError> {
//@stub
    todo!()
//@sol
    match (op, v) {
        (UnOp::Neg, Value::Int(n)) => n.checked_neg().map(Value::Int).ok_or(RuntimeError::Overflow),
        (UnOp::Not, Value::Bool(b)) => Ok(Value::Bool(!b)),
        (UnOp::Neg, _) => Err(RuntimeError::TypeMismatch("`-` needs an integer".to_string())),
        (UnOp::Not, _) => Err(RuntimeError::TypeMismatch("`!` needs a boolean".to_string())),
    }
//@foot
}
//@endtask

//@task 4
/// Arithmetic and ordering need two integers; `==` and `!=` accept two values of the same kind.
fn apply_binary(op: BinOp, a: &Value, b: &Value) -> Result<Value, RuntimeError> {
//@stub
    todo!()
//@sol
    match (a, b) {
        (Value::Int(x), Value::Int(y)) => {
            let (x, y) = (*x, *y);
            match op {
                BinOp::Add => x.checked_add(y).map(Value::Int).ok_or(RuntimeError::Overflow),
                BinOp::Sub => x.checked_sub(y).map(Value::Int).ok_or(RuntimeError::Overflow),
                BinOp::Mul => x.checked_mul(y).map(Value::Int).ok_or(RuntimeError::Overflow),
                BinOp::Div => {
                    if y == 0 {
                        Err(RuntimeError::DivByZero)
                    } else {
                        x.checked_div(y).map(Value::Int).ok_or(RuntimeError::Overflow)
                    }
                }
                BinOp::Rem => {
                    if y == 0 {
                        Err(RuntimeError::DivByZero)
                    } else {
                        x.checked_rem(y).map(Value::Int).ok_or(RuntimeError::Overflow)
                    }
                }
                BinOp::Eq => Ok(Value::Bool(x == y)),
                BinOp::Ne => Ok(Value::Bool(x != y)),
                BinOp::Lt => Ok(Value::Bool(x < y)),
                BinOp::Le => Ok(Value::Bool(x <= y)),
                BinOp::Gt => Ok(Value::Bool(x > y)),
                BinOp::Ge => Ok(Value::Bool(x >= y)),
            }
        }
        (Value::Bool(x), Value::Bool(y)) => match op {
            BinOp::Eq => Ok(Value::Bool(x == y)),
            BinOp::Ne => Ok(Value::Bool(x != y)),
            _ => Err(RuntimeError::TypeMismatch(format!("`{}` needs integers", op.symbol()))),
        },
        _ => Err(RuntimeError::TypeMismatch(format!("`{}` needs two values of the same kind", op.symbol()))),
    }
//@foot
}
//@endtask

//@if <6
//@task 4
/// Evaluate an expression. Operands are evaluated left to right. Variables do not exist yet,
/// and neither do calls.
fn eval(e: &Expr) -> Result<Value, RuntimeError> {
//@stub
    todo!()
//@sol
    match e {
        Expr::Num(n) => Ok(Value::Int(*n)),
        Expr::Bool(b) => Ok(Value::Bool(*b)),
        Expr::Var(name) => Err(RuntimeError::Undefined(name.clone())),
        Expr::Unary(op, inner) => apply_unary(*op, &eval(inner)?),
        Expr::Binary(op, l, r) => {
            let a = eval(l)?;
            let b = eval(r)?;
            apply_binary(*op, &a, &b)
        }
        Expr::Call(..) => Err(RuntimeError::NotCallable),
    }
//@foot
}
//@endtask
//@endif
//@endif

//@if >=5
// ================= One error type =================

#[derive(Debug, PartialEq, Clone)]
enum Error {
    Parse(ParseError),
    Runtime(RuntimeError),
}

impl From<ParseError> for Error {
    fn from(e: ParseError) -> Error {
        Error::Parse(e)
    }
}

impl From<RuntimeError> for Error {
    fn from(e: RuntimeError) -> Error {
        Error::Runtime(e)
    }
}
//@endif

//@if ==5
//@task 5
/// Lex, parse and evaluate one expression.
fn calc(src: &str) -> Result<Value, Error> {
//@stub
    todo!()
//@sol
    let expr = parse_expression(src)?;
    Ok(eval(&expr)?)
//@foot
}
//@endtask

//@task 5
/// One output line per non-blank input line (blank means only whitespace):
///   "N: VALUE"                      on success,
///   "N: error: parse error at POS: MESSAGE"   for a ParseError,
///   "N: error: TEXT"                for a RuntimeError (its Display text).
/// N counts lines from 1 and blank lines still count.
fn calc_lines(src: &str) -> Vec<String> {
//@stub
    todo!()
//@sol
    let mut out = Vec::new();
    for (i, line) in src.lines().enumerate() {
        if line.trim().is_empty() {
            continue;
        }
        let n = i + 1;
        out.push(match calc(line) {
            Ok(v) => format!("{}: {}", n, v),
            Err(Error::Parse(p)) => format!("{}: error: parse error at {}: {}", n, p.pos, p.msg),
            Err(Error::Runtime(r)) => format!("{}: error: {}", n, r),
        });
    }
    out
//@foot
}
//@endtask

//@task 5
/// The sum of every line that evaluates to an integer. Bools and errors are ignored.
/// None when no line gave an integer, or when the sum would overflow an i64.
fn calc_total(src: &str) -> Option<i64> {
//@stub
    todo!()
//@sol
    let mut total: Option<i64> = None;
    for line in src.lines() {
        if let Ok(Value::Int(n)) = calc(line) {
            total = Some(total.unwrap_or(0).checked_add(n)?);
        }
    }
    total
//@foot
}
//@endtask
//@endif

//@if >=6
// ================= Environments =================

struct Frame {
    vars: HashMap<String, Value>,
    parent: Option<Env>,
}

/// A scope: a table of variables with an optional enclosing scope. Cloning an `Env` gives
/// another handle to the same scope. All the scopes of one program share an output buffer.
#[derive(Clone)]
struct Env {
    frame: Rc<RefCell<Frame>>,
    out: Rc<RefCell<Vec<String>>>,
}

impl fmt::Debug for Env {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Env")
    }
}

impl PartialEq for Env {
    fn eq(&self, other: &Env) -> bool {
        Rc::ptr_eq(&self.frame, &other.frame)
    }
}

impl Env {
//@task 6
    /// A fresh global scope with an empty output buffer.
    fn new() -> Env {
//@stub
        todo!()
//@sol
        Env {
            frame: Rc::new(RefCell::new(Frame { vars: HashMap::new(), parent: None })),
            out: Rc::new(RefCell::new(Vec::new())),
        }
//@foot
    }
//@endtask

//@task 6
    /// A new, empty scope nested inside this one (sharing the output buffer).
    fn child(&self) -> Env {
//@stub
        todo!()
//@sol
        Env {
            frame: Rc::new(RefCell::new(Frame { vars: HashMap::new(), parent: Some(self.clone()) })),
            out: self.out.clone(),
        }
//@foot
    }
//@endtask

//@task 6
    /// Create (or overwrite) `name` in THIS scope.
    fn define(&self, name: &str, v: Value) {
//@stub
        todo!()
//@sol
        self.frame.borrow_mut().vars.insert(name.to_string(), v);
//@foot
    }
//@endtask

//@task 6
    /// Look `name` up here, then in the enclosing scopes, innermost first.
    fn get(&self, name: &str) -> Option<Value> {
//@stub
        todo!()
//@sol
        let frame = self.frame.borrow();
        match frame.vars.get(name) {
            Some(v) => Some(v.clone()),
            None => frame.parent.as_ref().and_then(|p| p.get(name)),
        }
//@foot
    }
//@endtask

//@task 6
    /// Overwrite the nearest existing `name`. Returns false when there is none.
    fn assign(&self, name: &str, v: Value) -> bool {
//@stub
        todo!()
//@sol
        let mut frame = self.frame.borrow_mut();
        if let Some(slot) = frame.vars.get_mut(name) {
            *slot = v;
            return true;
        }
        match &frame.parent {
            Some(p) => p.assign(name, v),
            None => false,
        }
//@foot
    }
//@endtask

//@task 6
    /// How many scopes there are from here to the global one, counting both (global is 1).
    fn depth(&self) -> usize {
//@stub
        todo!()
//@sol
        match &self.frame.borrow().parent {
            Some(p) => 1 + p.depth(),
            None => 1,
        }
//@foot
    }
//@endtask

    /// Add a line of program output.
    fn emit(&self, line: String) {
        self.out.borrow_mut().push(line);
    }

    /// Take and clear the output collected so far.
    fn take_output(&self) -> Vec<String> {
        std::mem::take(&mut *self.out.borrow_mut())
    }
}

//@if <9
fn eval(e: &Expr, env: &Env) -> Result<Value, RuntimeError> {
    match e {
        Expr::Num(n) => Ok(Value::Int(*n)),
        Expr::Bool(b) => Ok(Value::Bool(*b)),
        Expr::Var(name) => env.get(name).ok_or_else(|| RuntimeError::Undefined(name.clone())),
        Expr::Unary(op, inner) => apply_unary(*op, &eval(inner, env)?),
        Expr::Binary(op, l, r) => {
            let a = eval(l, env)?;
            let b = eval(r, env)?;
            apply_binary(*op, &a, &b)
        }
        Expr::Call(..) => Err(RuntimeError::NotCallable),
    }
}
//@else
fn eval(e: &Expr, env: &Env) -> Result<Value, RuntimeError> {
    match e {
        Expr::Num(n) => Ok(Value::Int(*n)),
        Expr::Bool(b) => Ok(Value::Bool(*b)),
        Expr::Var(name) => env.get(name).ok_or_else(|| RuntimeError::Undefined(name.clone())),
        Expr::Unary(op, inner) => apply_unary(*op, &eval(inner, env)?),
        Expr::Binary(op, l, r) => {
            let a = eval(l, env)?;
            let b = eval(r, env)?;
            apply_binary(*op, &a, &b)
        }
        Expr::Call(callee, args) => eval_call(callee, args, env),
    }
}
//@endif
//@endif

//@if >=7
// ================= Statements =================

#[derive(Debug, PartialEq, Clone)]
enum Stmt {
    Let(String, Expr),
    Assign(String, Expr),
    Print(Expr),
    ExprStmt(Expr),
//@if >=8
    Block(Vec<Stmt>),
    If(Expr, Vec<Stmt>, Option<Vec<Stmt>>),
    While(Expr, Vec<Stmt>),
//@endif
//@if >=9
    Fn(String, Vec<String>, Vec<Stmt>),
    Return(Option<Expr>),
//@endif
}

/// How a statement finished: normally, or by `return`ing a value.
#[derive(Debug, PartialEq, Clone)]
enum Flow {
    Normal,
    Return(Value),
}

//@task 7
/// Parse one statement.
///   let NAME = EXPR ;      NAME = EXPR ;      print EXPR ;      EXPR ;
fn parse_stmt(s: &mut Stream) -> Result<Stmt, ParseError> {
//@stub
    todo!()
//@sol
    match s.peek() {
        Some(Token::Let) => {
            s.next();
            let name = s.expect_ident()?;
            s.expect(&Token::Eq)?;
            let e = parse_expr(s, 0)?;
            s.expect(&Token::Semi)?;
            Ok(Stmt::Let(name, e))
        }
        Some(Token::Print) => {
            s.next();
            let e = parse_expr(s, 0)?;
            s.expect(&Token::Semi)?;
            Ok(Stmt::Print(e))
        }
//@if >=8
        Some(Token::LBrace) => Ok(Stmt::Block(parse_block(s)?)),
        Some(Token::If) => parse_if(s),
        Some(Token::While) => parse_while(s),
//@endif
//@if >=9
        Some(Token::Fn) => parse_fn(s),
        Some(Token::Return) => parse_return(s),
//@endif
        Some(Token::Ident(_)) if s.peek_at(1) == Some(&Token::Eq) => {
            let name = s.expect_ident()?;
            s.next();
            let e = parse_expr(s, 0)?;
            s.expect(&Token::Semi)?;
            Ok(Stmt::Assign(name, e))
        }
        _ => {
            let e = parse_expr(s, 0)?;
            s.expect(&Token::Semi)?;
            Ok(Stmt::ExprStmt(e))
        }
    }
//@foot
}
//@endtask

//@task 7
/// Run one statement in `env`.
fn exec_stmt(stmt: &Stmt, env: &Env) -> Result<Flow, RuntimeError> {
//@stub
    todo!()
//@sol
    match stmt {
        Stmt::Let(name, e) => {
            let v = eval(e, env)?;
            env.define(name, v);
            Ok(Flow::Normal)
        }
        Stmt::Assign(name, e) => {
            let v = eval(e, env)?;
            if env.assign(name, v) {
                Ok(Flow::Normal)
            } else {
                Err(RuntimeError::Undefined(name.clone()))
            }
        }
        Stmt::Print(e) => {
            let v = eval(e, env)?;
            env.emit(v.to_string());
            Ok(Flow::Normal)
        }
        Stmt::ExprStmt(e) => {
            eval(e, env)?;
            Ok(Flow::Normal)
        }
//@if >=8
        Stmt::Block(stmts) => exec_block(stmts, env),
        Stmt::If(cond, then, els) => exec_if(cond, then, els.as_deref(), env),
        Stmt::While(cond, body) => exec_while(cond, body, env),
//@endif
//@if >=9
        Stmt::Fn(name, params, body) => exec_fn(name, params, body, env),
        Stmt::Return(e) => exec_return(e.as_ref(), env),
//@endif
    }
//@foot
}
//@endtask

/// Run statements one after another in the same scope. Stops early on `return`.
fn exec_stmts(stmts: &[Stmt], env: &Env) -> Result<Flow, RuntimeError> {
    for s in stmts {
        if let Flow::Return(v) = exec_stmt(s, env)? {
            return Ok(Flow::Return(v));
        }
    }
    Ok(Flow::Normal)
}

fn parse_program(src: &str) -> Result<Vec<Stmt>, ParseError> {
    let mut s = Stream::from_source(src)?;
    let mut stmts = Vec::new();
    while !s.at_end() {
        stmts.push(parse_stmt(&mut s)?);
    }
    Ok(stmts)
}

/// Parse and run `src` in `env`. The output stays in the environment's buffer.
fn exec_source(src: &str, env: &Env) -> Result<(), Error> {
    let prog = parse_program(src)?;
    exec_stmts(&prog, env)?;
    Ok(())
}

/// Parse and run `src` in `env`, returning (and clearing) the printed lines.
fn run_in(src: &str, env: &Env) -> Result<Vec<String>, Error> {
    exec_source(src, env)?;
    Ok(env.take_output())
}

/// Run a program in a fresh global scope and return what it printed.
fn run(src: &str) -> Result<Vec<String>, Error> {
    run_in(src, &Env::new())
}
//@endif

//@if >=8
//@task 8
/// `{ stmt* }` -- the statements between the braces.
fn parse_block(s: &mut Stream) -> Result<Vec<Stmt>, ParseError> {
//@stub
    todo!()
//@sol
    s.expect(&Token::LBrace)?;
    let mut stmts = Vec::new();
    while !s.eat(&Token::RBrace) {
        if s.at_end() {
            return Err(s.expect(&Token::RBrace).unwrap_err());
        }
        stmts.push(parse_stmt(s)?);
    }
    Ok(stmts)
//@foot
}
//@endtask

//@task 8
/// `if EXPR { ... }` with an optional `else { ... }` or `else if ...` (which becomes an
/// else-branch holding the single nested `if`).
fn parse_if(s: &mut Stream) -> Result<Stmt, ParseError> {
//@stub
    todo!()
//@sol
    s.expect(&Token::If)?;
    let cond = parse_expr(s, 0)?;
    let then = parse_block(s)?;
    let els = if s.eat(&Token::Else) {
        if s.peek() == Some(&Token::If) {
            Some(vec![parse_if(s)?])
        } else {
            Some(parse_block(s)?)
        }
    } else {
        None
    };
    Ok(Stmt::If(cond, then, els))
//@foot
}
//@endtask

//@task 8
/// `while EXPR { ... }`
fn parse_while(s: &mut Stream) -> Result<Stmt, ParseError> {
//@stub
    todo!()
//@sol
    s.expect(&Token::While)?;
    let cond = parse_expr(s, 0)?;
    let body = parse_block(s)?;
    Ok(Stmt::While(cond, body))
//@foot
}
//@endtask

//@task 8
/// Run `stmts` in a new scope nested in `env`.
fn exec_block(stmts: &[Stmt], env: &Env) -> Result<Flow, RuntimeError> {
//@stub
    todo!()
//@sol
    exec_stmts(stmts, &env.child())
//@foot
}
//@endtask

//@task 8
/// The condition must evaluate to a boolean (anything else is a TypeMismatch).
/// Each branch runs in its own scope.
fn exec_if(cond: &Expr, then: &[Stmt], els: Option<&[Stmt]>, env: &Env) -> Result<Flow, RuntimeError> {
//@stub
    todo!()
//@sol
    match eval(cond, env)? {
        Value::Bool(true) => exec_block(then, env),
        Value::Bool(false) => match els {
            Some(branch) => exec_block(branch, env),
            None => Ok(Flow::Normal),
        },
        _ => Err(RuntimeError::TypeMismatch("condition must be a boolean".to_string())),
    }
//@foot
}
//@endtask

//@task 8
/// Re-check the condition before every pass; each pass runs the body in a fresh scope.
fn exec_while(cond: &Expr, body: &[Stmt], env: &Env) -> Result<Flow, RuntimeError> {
//@stub
    todo!()
//@sol
    loop {
        match eval(cond, env)? {
            Value::Bool(true) => {
                if let Flow::Return(v) = exec_block(body, env)? {
                    return Ok(Flow::Return(v));
                }
            }
            Value::Bool(false) => return Ok(Flow::Normal),
            _ => return Err(RuntimeError::TypeMismatch("condition must be a boolean".to_string())),
        }
    }
//@foot
}
//@endtask
//@endif

//@if >=9
// ================= Functions and closures =================

/// A user function together with the scope it was defined in.
struct Function {
    name: String,
    params: Vec<String>,
    body: Vec<Stmt>,
    env: Env,
}

impl fmt::Debug for Function {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "<fn {}>", self.name)
    }
}

impl PartialEq for Function {
    fn eq(&self, other: &Function) -> bool {
        std::ptr::eq(self, other)
    }
}

//@task 9
/// `fn NAME ( PARAM, ... ) { ... }`
fn parse_fn(s: &mut Stream) -> Result<Stmt, ParseError> {
//@stub
    todo!()
//@sol
    s.expect(&Token::Fn)?;
    let name = s.expect_ident()?;
    s.expect(&Token::LParen)?;
    let mut params = Vec::new();
    if !s.eat(&Token::RParen) {
        loop {
            params.push(s.expect_ident()?);
            if s.eat(&Token::Comma) {
                continue;
            }
            s.expect(&Token::RParen)?;
            break;
        }
    }
    let body = parse_block(s)?;
    Ok(Stmt::Fn(name, params, body))
//@foot
}
//@endtask

//@task 9
/// `return ;` or `return EXPR ;`
fn parse_return(s: &mut Stream) -> Result<Stmt, ParseError> {
//@stub
    todo!()
//@sol
    s.expect(&Token::Return)?;
    if s.eat(&Token::Semi) {
        return Ok(Stmt::Return(None));
    }
    let e = parse_expr(s, 0)?;
    s.expect(&Token::Semi)?;
    Ok(Stmt::Return(Some(e)))
//@foot
}
//@endtask

//@task 9
/// Executing a `fn` statement creates a function value that remembers `env`, and defines
/// it under its name in `env`.
fn exec_fn(name: &str, params: &[String], body: &[Stmt], env: &Env) -> Result<Flow, RuntimeError> {
//@stub
    todo!()
//@sol
    let f = Function {
        name: name.to_string(),
        params: params.to_vec(),
        body: body.to_vec(),
        env: env.clone(),
    };
    env.define(name, Value::Func(Rc::new(f)));
    Ok(Flow::Normal)
//@foot
}
//@endtask

//@task 9
/// `return` finishes with `Flow::Return(value)`; a bare `return` carries `Value::Nil`.
fn exec_return(e: Option<&Expr>, env: &Env) -> Result<Flow, RuntimeError> {
//@stub
    todo!()
//@sol
    let v = match e {
        Some(x) => eval(x, env)?,
        None => Value::Nil,
    };
    Ok(Flow::Return(v))
//@foot
}
//@endtask

//@task 9
/// Call a user function. A wrong number of arguments is `RuntimeError::Arity`. The body
/// runs in a new scope nested in the function's own `env` (not the caller's), with the
/// parameters defined in it. The result is the returned value, or `Nil` if the body just ends.
fn call_function(f: &Function, args: Vec<Value>) -> Result<Value, RuntimeError> {
//@stub
    todo!()
//@sol
    if args.len() != f.params.len() {
        return Err(RuntimeError::Arity { expected: f.params.len(), got: args.len() });
    }
    let local = f.env.child();
    for (p, a) in f.params.iter().zip(args) {
        local.define(p, a);
    }
    match exec_stmts(&f.body, &local)? {
        Flow::Return(v) => Ok(v),
        Flow::Normal => Ok(Value::Nil),
    }
//@foot
}
//@endtask

//@task 9
/// Evaluate the callee, then the arguments left to right. If the callee is not a function
/// value that is `NotCallable`; otherwise call it.
fn eval_call(callee: &Expr, args: &[Expr], env: &Env) -> Result<Value, RuntimeError> {
//@stub
    todo!()
//@sol
    let f = eval(callee, env)?;
    let mut vals = Vec::new();
    for a in args {
        vals.push(eval(a, env)?);
    }
    match f {
        Value::Func(func) => call_function(&func, vals),
        _ => Err(RuntimeError::NotCallable),
    }
//@foot
}
//@endtask
//@endif

//@if >=10
// ================= Error reports =================

//@task 10
/// Line and column (both counting from 1) of byte offset `pos`. Columns count characters.
/// An offset at or past the end of the source is the position just after the last character.
fn line_col(src: &str, pos: usize) -> (usize, usize) {
//@stub
    todo!()
//@sol
    let mut line = 1;
    let mut col = 1;
    for (i, c) in src.char_indices() {
        if i >= pos {
            break;
        }
        if c == '\n' {
            line += 1;
            col = 1;
        } else {
            col += 1;
        }
    }
    (line, col)
//@foot
}
//@endtask

//@task 10
/// Turn an error into text for a person.
///   Runtime:  "runtime error: " + the error's Display text
///   Parse:    four lines (no trailing newline):
///             error: MESSAGE
///               at line L, column C
///             L | the source line
///               | spaces then ^        (aligned under the column)
/// The gutter ("L") is as wide as the line number, so the `|` signs line up.
fn render_error(src: &str, err: &Error) -> String {
//@stub
    todo!()
//@sol
    match err {
        Error::Runtime(r) => format!("runtime error: {}", r),
        Error::Parse(p) => {
            let (line, col) = line_col(src, p.pos);
            let text = src.lines().nth(line - 1).unwrap_or("");
            let num = line.to_string();
            format!(
                "error: {}\n  at line {}, column {}\n{} | {}\n{} | {}^",
                p.msg,
                line,
                col,
                num,
                text,
                " ".repeat(num.len()),
                " ".repeat(col - 1)
            )
        }
    }
//@foot
}
//@endtask

//@task 10
/// Run `src`. On success the printed lines joined with "\n"; on failure `render_error`.
fn report(src: &str) -> String {
//@stub
    todo!()
//@sol
    match run(src) {
        Ok(lines) => lines.join("\n"),
        Err(e) => render_error(src, &e),
    }
//@foot
}
//@endtask
//@endif

//@if >=11
// ================= A session =================

/// A little read-eval-print loop: the environment lives as long as the session.
struct Session {
    env: Env,
    ok_lines: usize,
}

impl Session {
//@task 11
    fn new() -> Session {
//@stub
        todo!()
//@sol
        Session { env: Env::new(), ok_lines: 0 }
//@foot
    }
//@endtask

//@task 11
    /// How many lines have run without an error so far.
    fn ok_lines(&self) -> usize {
//@stub
        todo!()
//@sol
        self.ok_lines
//@foot
    }
//@endtask

//@task 11
    /// Handle one line and return the response text (see the task for the exact rules).
    fn eval_line(&mut self, line: &str) -> String {
//@stub
        todo!()
//@sol
        if line.trim().is_empty() {
            return String::new();
        }
        let result: Result<Option<Value>, Error> = match parse_expression(line) {
            Ok(expr) => eval(&expr, &self.env).map(Some).map_err(Error::from),
            Err(_) => exec_source(line, &self.env).map(|_| None),
        };
        let mut lines = self.env.take_output();
        match result {
            Ok(Some(v)) => {
                lines.push(v.to_string());
                self.ok_lines += 1;
            }
            Ok(None) => self.ok_lines += 1,
            Err(e) => lines.push(render_error(line, &e)),
        }
        lines.join("\n")
//@foot
    }
//@endtask
}

//@task 11
/// Feed `lines` to one fresh session. For each line the transcript has "> LINE" and then,
/// if the response is not empty, the response. Everything joined with "\n".
fn transcript(lines: &[&str]) -> String {
//@stub
    todo!()
//@sol
    let mut session = Session::new();
    let mut out: Vec<String> = Vec::new();
    for line in lines {
        out.push(format!("> {}", line));
        let response = session.eval_line(line);
        if !response.is_empty() {
            out.push(response);
        }
    }
    out.join("\n")
//@foot
}
//@endtask
//@endif

//@if >=12
// ================= Bytecode =================

#[derive(Debug, PartialEq, Clone)]
enum Op {
    Const(i64),
    Bool(bool),
    Unary(UnOp),
    Binary(BinOp),
    Load(String),
//@if >=13
    Store(String),  // pop a value and define a global
    Assign(String), // pop a value and overwrite an existing global
    Pop,
    Print,
    Jump(usize),
    JumpIfFalse(usize),
//@endif
}

#[derive(Debug, PartialEq, Clone)]
enum CompileError {
    Unsupported(String),
}

//@task 12
/// Compile an expression to postfix code: operands first, then the operator.
/// A call is `Err(CompileError::Unsupported(..))`.
fn compile_expr(e: &Expr) -> Result<Vec<Op>, CompileError> {
//@stub
    todo!()
//@sol
    match e {
        Expr::Num(n) => Ok(vec![Op::Const(*n)]),
        Expr::Bool(b) => Ok(vec![Op::Bool(*b)]),
        Expr::Var(name) => Ok(vec![Op::Load(name.clone())]),
        Expr::Unary(op, x) => {
            let mut code = compile_expr(x)?;
            code.push(Op::Unary(*op));
            Ok(code)
        }
        Expr::Binary(op, l, r) => {
            let mut code = compile_expr(l)?;
            code.extend(compile_expr(r)?);
            code.push(Op::Binary(*op));
            Ok(code)
        }
        Expr::Call(..) => Err(CompileError::Unsupported("calls".to_string())),
    }
//@foot
}
//@endtask

//@if ==12
//@task 12
/// The deepest the value stack gets while `code` runs (straight-line code, no jumps).
/// Const, Bool and Load push one value; Unary replaces the top one; Binary replaces two by one.
fn max_stack(code: &[Op]) -> usize {
//@stub
    todo!()
//@sol
    let mut depth: usize = 0;
    let mut deepest = 0;
    for op in code {
        match op {
            Op::Const(_) | Op::Bool(_) | Op::Load(_) => depth += 1,
            Op::Unary(_) => {}
            Op::Binary(_) => depth -= 1,
        }
        deepest = deepest.max(depth);
    }
    deepest
//@foot
}
//@endtask
//@endif
//@endif

//@if >=13
// ================= The virtual machine =================

struct Vm {
    stack: Vec<Value>,
    globals: HashMap<String, Value>,
    out: Vec<String>,
}

impl Vm {
    fn new() -> Vm {
        Vm { stack: Vec::new(), globals: HashMap::new(), out: Vec::new() }
    }

    fn pop(&mut self) -> Result<Value, RuntimeError> {
        self.stack.pop().ok_or_else(|| RuntimeError::TypeMismatch("stack underflow".to_string()))
    }

//@task 13
    /// Run `code` from the first instruction until the instruction pointer runs off the end.
    fn run(&mut self, code: &[Op]) -> Result<(), RuntimeError> {
//@stub
        todo!()
//@sol
        let mut ip = 0;
        while ip < code.len() {
            let op = &code[ip];
            ip += 1;
            match op {
                Op::Const(n) => self.stack.push(Value::Int(*n)),
                Op::Bool(b) => self.stack.push(Value::Bool(*b)),
                Op::Unary(u) => {
                    let v = self.pop()?;
                    self.stack.push(apply_unary(*u, &v)?);
                }
                Op::Binary(b) => {
                    let r = self.pop()?;
                    let l = self.pop()?;
                    self.stack.push(apply_binary(*b, &l, &r)?);
                }
                Op::Load(name) => {
                    let v = self
                        .globals
                        .get(name)
                        .cloned()
                        .ok_or_else(|| RuntimeError::Undefined(name.clone()))?;
                    self.stack.push(v);
                }
                Op::Store(name) => {
                    let v = self.pop()?;
                    self.globals.insert(name.clone(), v);
                }
                Op::Assign(name) => {
                    let v = self.pop()?;
                    match self.globals.get_mut(name) {
                        Some(slot) => *slot = v,
                        None => return Err(RuntimeError::Undefined(name.clone())),
                    }
                }
                Op::Pop => {
                    self.pop()?;
                }
                Op::Print => {
                    let v = self.pop()?;
                    self.out.push(v.to_string());
                }
                Op::Jump(target) => ip = *target,
                Op::JumpIfFalse(target) => match self.pop()? {
                    Value::Bool(false) => ip = *target,
                    Value::Bool(true) => {}
                    _ => return Err(RuntimeError::TypeMismatch("condition must be a boolean".to_string())),
                },
            }
        }
        Ok(())
//@foot
    }
//@endtask
}

/// Compile an expression and run it on a fresh VM; the result is what is left on the stack.
fn eval_via_vm(src: &str) -> Result<Value, Error> {
    let expr = parse_expression(src)?;
    let code = compile_expr(&expr).map_err(|e| Error::Runtime(RuntimeError::TypeMismatch(format!("{:?}", e))))?;
    let mut vm = Vm::new();
    vm.run(&code)?;
    vm.pop().map_err(Error::from)
}
//@endif

//@if >=14
// ================= Compiling statements =================

//@task 14
/// Append the code for one statement to `code`. Jump targets are absolute indices into
/// `code`, so use `code.len()` to learn where the next instruction will go.
///
///   let / assign / print / expression statement: the expression, then Store / Assign / Print / Pop
///   block:  its statements in order (this VM has one flat scope)
///   if:     cond, JumpIfFalse(else-or-end), then..., [Jump(end), else...]
///   while:  cond, JumpIfFalse(end), body..., Jump(start of cond)
///   fn / return: Err(CompileError::Unsupported(..))
fn emit_stmt(stmt: &Stmt, code: &mut Vec<Op>) -> Result<(), CompileError> {
//@stub
    todo!()
//@sol
    match stmt {
        Stmt::Let(name, e) => {
            code.extend(compile_expr(e)?);
            code.push(Op::Store(name.clone()));
        }
        Stmt::Assign(name, e) => {
            code.extend(compile_expr(e)?);
            code.push(Op::Assign(name.clone()));
        }
        Stmt::Print(e) => {
            code.extend(compile_expr(e)?);
            code.push(Op::Print);
        }
        Stmt::ExprStmt(e) => {
            code.extend(compile_expr(e)?);
            code.push(Op::Pop);
        }
        Stmt::Block(stmts) => {
            for s in stmts {
                emit_stmt(s, code)?;
            }
        }
        Stmt::If(cond, then, els) => {
            code.extend(compile_expr(cond)?);
            let jump_false = code.len();
            code.push(Op::JumpIfFalse(0));
            for s in then {
                emit_stmt(s, code)?;
            }
            match els {
                None => {
                    let end = code.len();
                    code[jump_false] = Op::JumpIfFalse(end);
                }
                Some(branch) => {
                    let jump_end = code.len();
                    code.push(Op::Jump(0));
                    let else_start = code.len();
                    code[jump_false] = Op::JumpIfFalse(else_start);
                    for s in branch {
                        emit_stmt(s, code)?;
                    }
                    let end = code.len();
                    code[jump_end] = Op::Jump(end);
                }
            }
        }
        Stmt::While(cond, body) => {
            let start = code.len();
            code.extend(compile_expr(cond)?);
            let jump_false = code.len();
            code.push(Op::JumpIfFalse(0));
            for s in body {
                emit_stmt(s, code)?;
            }
            code.push(Op::Jump(start));
            let end = code.len();
            code[jump_false] = Op::JumpIfFalse(end);
        }
        Stmt::Fn(..) | Stmt::Return(_) => {
            return Err(CompileError::Unsupported("functions".to_string()));
        }
    }
    Ok(())
//@foot
}
//@endtask

fn compile_stmts(stmts: &[Stmt]) -> Result<Vec<Op>, CompileError> {
    let mut code = Vec::new();
    for s in stmts {
        emit_stmt(s, &mut code)?;
    }
    Ok(code)
}

/// Parse, compile and run a program on the VM. Errors come back as the same text
/// `render_error` gives, so they can be compared with the tree-walking interpreter.
fn run_vm(src: &str) -> Result<Vec<String>, String> {
    let prog = parse_program(src).map_err(|e| render_error(src, &Error::Parse(e)))?;
    let code = compile_stmts(&prog).map_err(|e| format!("compile error: {:?}", e))?;
    let mut vm = Vm::new();
    vm.run(&code).map_err(|e| render_error(src, &Error::Runtime(e)))?;
    Ok(vm.out)
}
//@endif
