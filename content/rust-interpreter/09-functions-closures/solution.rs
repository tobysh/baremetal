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

fn lex(src: &str) -> Result<Vec<Spanned>, LexError> {
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
}

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

    /// The next token, without consuming it.
    fn peek(&self) -> Option<&Token> {
        self.toks.get(self.i).map(|s| &s.tok)
    }

    /// The token `n` places ahead (`peek_at(0)` is `peek()`).
    fn peek_at(&self, n: usize) -> Option<&Token> {
        self.toks.get(self.i + n).map(|s| &s.tok)
    }

    /// Byte offset of the next token, or the end of the source when there are no more.
    fn pos(&self) -> usize {
        self.toks.get(self.i).map_or(self.end, |s| s.pos)
    }

    /// Consume and return the next token.
    fn next(&mut self) -> Option<Spanned> {
        let t = self.toks.get(self.i).cloned();
        if t.is_some() {
            self.i += 1;
        }
        t
    }

    /// Consume the next token only if it equals `t`. Says whether it did.
    fn eat(&mut self, t: &Token) -> bool {
        if self.peek() == Some(t) {
            self.i += 1;
            true
        } else {
            false
        }
    }

    /// Consume `t` or fail. On success returns the token's position.
    fn expect(&mut self, t: &Token) -> Result<usize, ParseError> {
        let pos = self.pos();
        match self.peek() {
            Some(found) if found == t => {
                self.i += 1;
                Ok(pos)
            }
            Some(found) => Err(ParseError::new(format!("expected {:?}, found {:?}", t, found), pos)),
            None => Err(ParseError::new(format!("expected {:?}, found end of input", t), pos)),
        }
    }

    /// Consume an identifier and return its name.
    fn expect_ident(&mut self) -> Result<String, ParseError> {
        let pos = self.pos();
        match self.peek().cloned() {
            Some(Token::Ident(name)) => {
                self.i += 1;
                Ok(name)
            }
            Some(found) => Err(ParseError::new(format!("expected identifier, found {:?}", found), pos)),
            None => Err(ParseError::new("expected identifier, found end of input", pos)),
        }
    }

    fn at_end(&self) -> bool {
        self.i >= self.toks.len()
    }
}

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

/// Parse one expression, stopping before any operator that binds more loosely than `min_bp`.
fn parse_expr(s: &mut Stream, min_bp: u8) -> Result<Expr, ParseError> {
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
}

/// Parse a whole source text as one expression.
fn parse_expression(src: &str) -> Result<Expr, ParseError> {
    let mut s = Stream::from_source(src)?;
    let e = parse_expr(&mut s, 0)?;
    match s.next() {
        None => Ok(e),
        Some(t) => Err(ParseError::new(format!("unexpected {:?}", t.tok), t.pos)),
    }
}

// ================= Values and evaluation =================

#[derive(Debug, PartialEq, Clone)]
enum Value {
    Int(i64),
    Bool(bool),
    Nil,
    Func(Rc<Function>),
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Value::Int(n) => write!(f, "{}", n),
            Value::Bool(b) => write!(f, "{}", b),
            Value::Nil => write!(f, "nil"),
            Value::Func(func) => write!(f, "<fn {}>", func.name),
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

/// `-` on an integer (overflow is an error), `!` on a boolean. Anything else is a type error.
fn apply_unary(op: UnOp, v: &Value) -> Result<Value, RuntimeError> {
    match (op, v) {
        (UnOp::Neg, Value::Int(n)) => n.checked_neg().map(Value::Int).ok_or(RuntimeError::Overflow),
        (UnOp::Not, Value::Bool(b)) => Ok(Value::Bool(!b)),
        (UnOp::Neg, _) => Err(RuntimeError::TypeMismatch("`-` needs an integer".to_string())),
        (UnOp::Not, _) => Err(RuntimeError::TypeMismatch("`!` needs a boolean".to_string())),
    }
}

/// Arithmetic and ordering need two integers; `==` and `!=` accept two values of the same kind.
fn apply_binary(op: BinOp, a: &Value, b: &Value) -> Result<Value, RuntimeError> {
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
}

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
    /// A fresh global scope with an empty output buffer.
    fn new() -> Env {
        Env {
            frame: Rc::new(RefCell::new(Frame { vars: HashMap::new(), parent: None })),
            out: Rc::new(RefCell::new(Vec::new())),
        }
    }

    /// A new, empty scope nested inside this one (sharing the output buffer).
    fn child(&self) -> Env {
        Env {
            frame: Rc::new(RefCell::new(Frame { vars: HashMap::new(), parent: Some(self.clone()) })),
            out: self.out.clone(),
        }
    }

    /// Create (or overwrite) `name` in THIS scope.
    fn define(&self, name: &str, v: Value) {
        self.frame.borrow_mut().vars.insert(name.to_string(), v);
    }

    /// Look `name` up here, then in the enclosing scopes, innermost first.
    fn get(&self, name: &str) -> Option<Value> {
        let frame = self.frame.borrow();
        match frame.vars.get(name) {
            Some(v) => Some(v.clone()),
            None => frame.parent.as_ref().and_then(|p| p.get(name)),
        }
    }

    /// Overwrite the nearest existing `name`. Returns false when there is none.
    fn assign(&self, name: &str, v: Value) -> bool {
        let mut frame = self.frame.borrow_mut();
        if let Some(slot) = frame.vars.get_mut(name) {
            *slot = v;
            return true;
        }
        match &frame.parent {
            Some(p) => p.assign(name, v),
            None => false,
        }
    }

    /// How many scopes there are from here to the global one, counting both (global is 1).
    fn depth(&self) -> usize {
        match &self.frame.borrow().parent {
            Some(p) => 1 + p.depth(),
            None => 1,
        }
    }

    /// Add a line of program output.
    fn emit(&self, line: String) {
        self.out.borrow_mut().push(line);
    }

    /// Take and clear the output collected so far.
    fn take_output(&self) -> Vec<String> {
        std::mem::take(&mut *self.out.borrow_mut())
    }
}

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

// ================= Statements =================

#[derive(Debug, PartialEq, Clone)]
enum Stmt {
    Let(String, Expr),
    Assign(String, Expr),
    Print(Expr),
    ExprStmt(Expr),
    Block(Vec<Stmt>),
    If(Expr, Vec<Stmt>, Option<Vec<Stmt>>),
    While(Expr, Vec<Stmt>),
    Fn(String, Vec<String>, Vec<Stmt>),
    Return(Option<Expr>),
}

/// How a statement finished: normally, or by `return`ing a value.
#[derive(Debug, PartialEq, Clone)]
enum Flow {
    Normal,
    Return(Value),
}

/// Parse one statement.
///   let NAME = EXPR ;      NAME = EXPR ;      print EXPR ;      EXPR ;
fn parse_stmt(s: &mut Stream) -> Result<Stmt, ParseError> {
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
        Some(Token::LBrace) => Ok(Stmt::Block(parse_block(s)?)),
        Some(Token::If) => parse_if(s),
        Some(Token::While) => parse_while(s),
        Some(Token::Fn) => parse_fn(s),
        Some(Token::Return) => parse_return(s),
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
}

/// Run one statement in `env`.
fn exec_stmt(stmt: &Stmt, env: &Env) -> Result<Flow, RuntimeError> {
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
        Stmt::Block(stmts) => exec_block(stmts, env),
        Stmt::If(cond, then, els) => exec_if(cond, then, els.as_deref(), env),
        Stmt::While(cond, body) => exec_while(cond, body, env),
        Stmt::Fn(name, params, body) => exec_fn(name, params, body, env),
        Stmt::Return(e) => exec_return(e.as_ref(), env),
    }
}

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

/// `{ stmt* }` -- the statements between the braces.
fn parse_block(s: &mut Stream) -> Result<Vec<Stmt>, ParseError> {
    s.expect(&Token::LBrace)?;
    let mut stmts = Vec::new();
    while !s.eat(&Token::RBrace) {
        if s.at_end() {
            return Err(s.expect(&Token::RBrace).unwrap_err());
        }
        stmts.push(parse_stmt(s)?);
    }
    Ok(stmts)
}

/// `if EXPR { ... }` with an optional `else { ... }` or `else if ...` (which becomes an
/// else-branch holding the single nested `if`).
fn parse_if(s: &mut Stream) -> Result<Stmt, ParseError> {
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
}

/// `while EXPR { ... }`
fn parse_while(s: &mut Stream) -> Result<Stmt, ParseError> {
    s.expect(&Token::While)?;
    let cond = parse_expr(s, 0)?;
    let body = parse_block(s)?;
    Ok(Stmt::While(cond, body))
}

/// Run `stmts` in a new scope nested in `env`.
fn exec_block(stmts: &[Stmt], env: &Env) -> Result<Flow, RuntimeError> {
    exec_stmts(stmts, &env.child())
}

/// The condition must evaluate to a boolean (anything else is a TypeMismatch).
/// Each branch runs in its own scope.
fn exec_if(cond: &Expr, then: &[Stmt], els: Option<&[Stmt]>, env: &Env) -> Result<Flow, RuntimeError> {
    match eval(cond, env)? {
        Value::Bool(true) => exec_block(then, env),
        Value::Bool(false) => match els {
            Some(branch) => exec_block(branch, env),
            None => Ok(Flow::Normal),
        },
        _ => Err(RuntimeError::TypeMismatch("condition must be a boolean".to_string())),
    }
}

/// Re-check the condition before every pass; each pass runs the body in a fresh scope.
fn exec_while(cond: &Expr, body: &[Stmt], env: &Env) -> Result<Flow, RuntimeError> {
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
}

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

/// `fn NAME ( PARAM, ... ) { ... }`
fn parse_fn(s: &mut Stream) -> Result<Stmt, ParseError> {
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
}

/// `return ;` or `return EXPR ;`
fn parse_return(s: &mut Stream) -> Result<Stmt, ParseError> {
    s.expect(&Token::Return)?;
    if s.eat(&Token::Semi) {
        return Ok(Stmt::Return(None));
    }
    let e = parse_expr(s, 0)?;
    s.expect(&Token::Semi)?;
    Ok(Stmt::Return(Some(e)))
}

/// Executing a `fn` statement creates a function value that remembers `env`, and defines
/// it under its name in `env`.
fn exec_fn(name: &str, params: &[String], body: &[Stmt], env: &Env) -> Result<Flow, RuntimeError> {
    let f = Function {
        name: name.to_string(),
        params: params.to_vec(),
        body: body.to_vec(),
        env: env.clone(),
    };
    env.define(name, Value::Func(Rc::new(f)));
    Ok(Flow::Normal)
}

/// `return` finishes with `Flow::Return(value)`; a bare `return` carries `Value::Nil`.
fn exec_return(e: Option<&Expr>, env: &Env) -> Result<Flow, RuntimeError> {
    let v = match e {
        Some(x) => eval(x, env)?,
        None => Value::Nil,
    };
    Ok(Flow::Return(v))
}

/// Call a user function. A wrong number of arguments is `RuntimeError::Arity`. The body
/// runs in a new scope nested in the function's own `env` (not the caller's), with the
/// parameters defined in it. The result is the returned value, or `Nil` if the body just ends.
fn call_function(f: &Function, args: Vec<Value>) -> Result<Value, RuntimeError> {
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
}

/// Evaluate the callee, then the arguments left to right. If the callee is not a function
/// value that is `NotCallable`; otherwise call it.
fn eval_call(callee: &Expr, args: &[Expr], env: &Env) -> Result<Value, RuntimeError> {
    let f = eval(callee, env)?;
    let mut vals = Vec::new();
    for a in args {
        vals.push(eval(a, env)?);
    }
    match f {
        Value::Func(func) => call_function(&func, vals),
        _ => Err(RuntimeError::NotCallable),
    }
}

// Try it with Run.
fn main() {
    let src = "fn make_counter() {
    let n = 0;
    fn next() { n = n + 1; return n; }
    return next;
}
let c = make_counter();
c(); c();
print c();";
    println!("{:?}", run(src));
}
