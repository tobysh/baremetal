#[test]
fn dot_products() {
    assert_eq!(dot(&[1, 2, 3], &[4, 5, 6]), 32, "1*4 + 2*5 + 3*6");
    assert_eq!(dot(&[-2, 7], &[5, 3]), 11, "negative values: -10 + 21");
    assert_eq!(dot(&[], &[]), 0, "empty slices");
}

#[test]
fn dot_uses_the_overlap() {
    assert_eq!(dot(&[1, 1, 1, 1], &[2, 3]), 5, "only two pairs exist");
    assert_eq!(dot(&[5], &[1, 2, 3]), 5, "only one pair exists");
}

#[test]
fn running_max_cases() {
    assert_eq!(running_max(&[3, 1, 4, 1, 5]), vec![3, 3, 4, 4, 5], "mixed values");
    assert_eq!(running_max(&[-5, -9, -1]), vec![-5, -5, -1], "all negative");
    assert_eq!(running_max(&[9, 8, 7]), vec![9, 9, 9], "falling values");
    assert_eq!(running_max(&[]), Vec::<i32>::new(), "empty input");
}

#[test]
fn pairwise_diffs_cases() {
    assert_eq!(pairwise_diffs(&[1, 4, 9, 16]), vec![3, 5, 7], "squares");
    assert_eq!(pairwise_diffs(&[10, 7, 7, 2]), vec![-3, 0, -5], "falls and flat steps");
    assert_eq!(pairwise_diffs(&[5]), Vec::<i32>::new(), "one item has no pairs");
    assert_eq!(pairwise_diffs(&[]), Vec::<i32>::new(), "empty input");
}

#[test]
fn chunk_sums_cases() {
    assert_eq!(chunk_sums(&[1, 2, 3, 4, 5], 2), vec![3, 7, 5], "short last chunk");
    assert_eq!(chunk_sums(&[1, 2, 3, 4, 5, 6], 3), vec![6, 15], "exact chunks");
    assert_eq!(chunk_sums(&[4, 4], 10), vec![8], "chunk bigger than input");
    assert_eq!(chunk_sums(&[1, 2, 3], 1), vec![1, 2, 3], "chunks of one");
    assert_eq!(chunk_sums(&[1, 2, 3], 0), Vec::<i32>::new(), "n = 0 gives nothing");
    assert_eq!(chunk_sums(&[], 4), Vec::<i32>::new(), "empty input");
}

#[test]
fn positions_cases() {
    assert_eq!(positions(&[1, 2, 1, 3, 1], 1), vec![0, 2, 4], "three hits");
    assert_eq!(positions(&[7, 8, 9], 8), vec![1], "one hit");
    assert_eq!(positions(&[7, 8, 9], 5), Vec::<usize>::new(), "no hit");
    assert_eq!(positions(&[], 0), Vec::<usize>::new(), "empty input");
}
