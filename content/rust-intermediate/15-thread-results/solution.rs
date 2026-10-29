use std::sync::Arc;
use std::thread;

fn run_guarded<F>(f: F) -> Result<i32, String>
where
    F: FnOnce() -> i32 + Send + 'static,
{
    match thread::spawn(f).join() {
        Ok(v) => Ok(v),
        Err(payload) => {
            let msg = if let Some(s) = payload.downcast_ref::<&str>() {
                s.to_string()
            } else if let Some(s) = payload.downcast_ref::<String>() {
                s.clone()
            } else {
                "unknown panic".to_string()
            };
            Err(msg)
        }
    }
}

fn collect_ordered<F>(n: usize, f: Arc<F>) -> Result<Vec<u32>, String>
where
    F: Fn(usize) -> Result<u32, String> + Send + Sync + 'static,
{
    let handles: Vec<_> = (0..n)
        .map(|i| {
            let f = Arc::clone(&f);
            thread::spawn(move || f(i))
        })
        .collect();
    let mut out = Vec::new();
    let mut first_err: Option<String> = None;
    for h in handles {
        match h.join() {
            Ok(Ok(v)) => out.push(v),
            Ok(Err(e)) => {
                if first_err.is_none() {
                    first_err = Some(e);
                }
            }
            Err(_) => {
                if first_err.is_none() {
                    first_err = Some("worker panicked".to_string());
                }
            }
        }
    }
    match first_err {
        Some(e) => Err(e),
        None => Ok(out),
    }
}

fn parse_chunks(chunks: Vec<Vec<String>>) -> Result<Vec<i64>, String> {
    let handles: Vec<_> = chunks
        .into_iter()
        .enumerate()
        .map(|(i, chunk)| {
            thread::spawn(move || -> Result<Vec<i64>, String> {
                chunk.iter().map(|s| s.trim().parse::<i64>().map_err(|e| format!("chunk {i}: {e}"))).collect()
            })
        })
        .collect();
    let mut all = Vec::new();
    let mut first_err: Option<String> = None;
    for h in handles {
        match h.join().expect("parser threads do not panic") {
            Ok(v) => all.extend(v),
            Err(e) => {
                if first_err.is_none() {
                    first_err = Some(e);
                }
            }
        }
    }
    match first_err {
        Some(e) => Err(e),
        None => Ok(all),
    }
}

// Try it with Run.
fn main() {
    println!("{:?}", run_guarded(|| 6 * 7));
    println!("{:?}", collect_ordered(4, Arc::new(|i| Ok(i as u32 * 2))));
    println!("{:?}", parse_chunks(vec![vec!["1".to_string(), "2".to_string()], vec!["x".to_string()]]));
}
