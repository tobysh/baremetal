#[test]
fn fan_out_sum_cases() {
    assert_eq!(fan_out_sum(vec![vec![1, 2, 3], vec![10, 20]]), 36, "two chunks");
    assert_eq!(fan_out_sum(vec![]), 0, "no chunks");
    assert_eq!(fan_out_sum(vec![vec![], vec![5], vec![]]), 5, "empty chunks are fine");
    let big: Vec<Vec<u64>> = (0..30).map(|i| (0..100).map(|j| i * 100 + j).collect()).collect();
    assert_eq!(fan_out_sum(big), (0..3000u64).sum::<u64>(), "30 chunks of 100");
}

#[test]
fn squares_in_order_cases() {
    assert_eq!(squares_in_order(vec![3, 1, 4]), vec![9, 1, 16], "small input");
    assert_eq!(squares_in_order(vec![]), Vec::<u64>::new(), "no input");
    assert_eq!(squares_in_order(vec![100_000]), vec![10_000_000_000], "the square needs 64 bits");
    let v: Vec<u32> = (0..64).rev().collect();
    let expect: Vec<u64> = v.iter().map(|&x| x as u64 * x as u64).collect();
    assert_eq!(squares_in_order(v), expect, "64 threads still give input order");
}

#[test]
fn producers_each_arrive_in_order() {
    let r = collect_from_producers(4, 50);
    assert_eq!(r.len(), 4, "one list per producer");
    let want: Vec<usize> = (0..50).collect();
    for (p, got) in r.iter().enumerate() {
        assert_eq!(got, &want, "producer {p}: messages from one sender arrive in the order sent");
    }
}

#[test]
fn producers_edge_cases() {
    assert_eq!(collect_from_producers(0, 5), Vec::<Vec<usize>>::new(), "no producers");
    assert_eq!(collect_from_producers(3, 0), vec![Vec::<usize>::new(); 3], "producers with nothing to send");
    assert_eq!(collect_from_producers(1, 3), vec![vec![0, 1, 2]], "a single producer");
}

#[test]
fn pipeline_cases() {
    assert_eq!(pipeline(vec![1, 2, 3]), vec![3, 5, 7], "double then add one");
    assert_eq!(pipeline(vec![]), Vec::<i32>::new(), "empty input");
    assert_eq!(pipeline(vec![-4, 0, 10]), vec![-7, 1, 21], "negatives and zero");
}

#[test]
fn pipeline_keeps_order_for_long_input() {
    let input: Vec<i32> = (0..500).collect();
    let expect: Vec<i32> = input.iter().map(|x| x * 2 + 1).collect();
    assert_eq!(pipeline(input), expect, "a single chain of stages preserves order");
}
