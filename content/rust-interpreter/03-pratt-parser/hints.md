Parse one prefix form into `lhs`, then loop over postfix calls or binary operators. Neither a comma nor `)` should be consumed by that loop.
===
For a binary operator recurse with its right power, then set `lhs = Expr::Binary(op, Box::new(lhs), Box::new(rhs))`. A call replaces `lhs` similarly and may be followed by another call.
