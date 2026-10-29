fn lines(v: &[&str]) -> Vec<String> {
    v.iter().map(|s| s.to_string()).collect()
}

#[test]
fn level_of_cases() {
    assert_eq!(level_of("ERROR disk full", 1), Ok("ERROR"), "a valid line");
    assert_eq!(level_of("   WARN  indented", 4), Ok("WARN"), "leading whitespace");
    assert_eq!(level_of("INFO", 2), Ok("INFO"), "a line with only the level");
    assert_eq!(level_of("info lower case", 7), Err(LogError::BadLine(7)), "case matters");
    assert_eq!(level_of("INFOX x", 3), Err(LogError::BadLine(3)), "the whole word must match");
    assert_eq!(level_of("", 9), Err(LogError::BadLine(9)), "empty line");
    assert_eq!(level_of("   ", 10), Err(LogError::BadLine(10)), "blank line");
}

#[test]
fn level_of_borrows_from_the_line() {
    let line = String::from("  WARN x");
    let level = level_of(&line, 1).unwrap();
    assert_eq!(level.as_ptr(), line[2..].as_ptr(), "the result is a slice of the line");
}

#[test]
fn display_message() {
    assert_eq!(LogError::BadLine(12).to_string(), "line 12: unknown level", "message");
    assert_eq!(LogError::BadLine(1).to_string(), "line 1: unknown level", "message for line 1");
}

#[test]
fn small_summary() {
    let m = summarize(lines(&["INFO a", "WARN b", "INFO c", "ERROR d", "INFO e"]), 2).unwrap();
    let v: Vec<(String, usize)> = m.into_iter().collect();
    assert_eq!(v, vec![("ERROR".to_string(), 1), ("INFO".to_string(), 3), ("WARN".to_string(), 1)], "counts by level");
}

#[test]
fn the_worker_count_does_not_change_the_answer() {
    let levels = ["INFO", "WARN", "ERROR", "INFO", "INFO", "ERROR", "WARN"];
    let input: Vec<String> = (0..103).map(|i| format!("{} message {}", levels[i % levels.len()], i)).collect();
    let base = summarize(input.clone(), 1).unwrap();
    assert_eq!(base.values().sum::<usize>(), 103, "every line is counted once");
    for w in [0, 2, 3, 7, 12] {
        assert_eq!(summarize(input.clone(), w).unwrap(), base, "same counts with {w} workers");
    }
}

#[test]
fn empty_input() {
    assert_eq!(summarize(vec![], 4).unwrap().len(), 0, "no lines, no counts");
}

#[test]
fn lowest_bad_line_wins_for_any_worker_count() {
    let mut input: Vec<String> = (0..100).map(|i| format!("INFO line {i}")).collect();
    input[36] = "oops".to_string();
    input[79] = "ERRORS plural".to_string();
    for w in [1, 2, 3, 4, 8] {
        for _ in 0..2 {
            assert_eq!(summarize(input.clone(), w), Err(LogError::BadLine(37)), "line 37 is the lowest bad line ({w} workers)");
        }
    }
}

#[test]
fn single_bad_line() {
    assert_eq!(summarize(lines(&["INFO ok", "WARN ok", ""]), 2), Err(LogError::BadLine(3)), "a blank last line");
    assert_eq!(summarize(lines(&["nope"]), 3), Err(LogError::BadLine(1)), "one line, bad");
}
