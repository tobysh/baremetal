use std::sync::Arc;
use std::thread;

fn run_guarded<F>(f: F) -> Result<i32, String>
where
    F: FnOnce() -> i32 + Send + 'static,
{
    Ok(0)
}

fn collect_ordered<F>(n: usize, f: Arc<F>) -> Result<Vec<u32>, String>
where
    F: Fn(usize) -> Result<u32, String> + Send + Sync + 'static,
{
    Ok(Vec::new())
}

fn parse_chunks(chunks: Vec<Vec<String>>) -> Result<Vec<i64>, String> {
    Ok(Vec::new())
}

// Try it with Run.
fn main() {
    println!("{:?}", run_guarded(|| 6 * 7));
    println!("{:?}", collect_ordered(4, Arc::new(|i| Ok(i as u32 * 2))));
    println!("{:?}", parse_chunks(vec![vec!["1".to_string(), "2".to_string()], vec!["x".to_string()]]));
}
