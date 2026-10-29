#[test]
fn josephus_orders() {
    assert_eq!(josephus(7, 3), vec![3, 6, 2, 7, 5, 1, 4], "the classic 7, 3");
    assert_eq!(josephus(5, 1), vec![1, 2, 3, 4, 5], "k = 1 removes in order");
    assert_eq!(josephus(4, 2), vec![2, 4, 3, 1], "every second person");
    assert_eq!(josephus(3, 10), vec![1, 3, 2], "k larger than the circle");
    assert_eq!(josephus(1, 5), vec![1], "one person");
    assert_eq!(josephus(0, 3), Vec::<usize>::new(), "nobody");
    assert_eq!(josephus(3, 0), Vec::<usize>::new(), "k = 0");
}

#[test]
fn smallest_values() {
    assert_eq!(k_smallest(&[9, 4, 7, 1, 8], 2), vec![1, 4], "two smallest");
    assert_eq!(k_smallest(&[5, 5, 5, 1], 3), vec![1, 5, 5], "duplicates count");
    assert_eq!(k_smallest(&[3, 2], 5), vec![2, 3], "k larger than input");
    assert_eq!(k_smallest(&[3, 2], 0), Vec::<i32>::new(), "k = 0");
    assert_eq!(k_smallest(&[-1, -8, 0], 2), vec![-8, -1], "negatives");
    assert_eq!(k_smallest(&[], 2), Vec::<i32>::new(), "empty");
}

#[test]
fn task_order() {
    let order = run_tasks(&[(1, "sweep"), (5, "deploy"), (5, "backup"), (3, "lint")]);
    assert_eq!(order, vec!["deploy", "backup", "lint", "sweep"], "priority, then name descending");
    assert_eq!(run_tasks(&[]), Vec::<String>::new(), "no tasks");
    assert_eq!(run_tasks(&[(2, "a"), (2, "b"), (2, "c")]), vec!["c", "b", "a"], "equal priority");
}

#[test]
fn sliding_maxima() {
    assert_eq!(window_maxima(&[1, 3, 2, 5, 4], 3), vec![3, 5, 5], "w = 3");
    assert_eq!(window_maxima(&[4, 3, 2, 1], 2), vec![4, 3, 2], "falling");
    assert_eq!(window_maxima(&[1, 2, 3, 4], 2), vec![2, 3, 4], "rising");
    assert_eq!(window_maxima(&[2, 2, 2], 2), vec![2, 2], "equal values");
    assert_eq!(window_maxima(&[7, 1], 1), vec![7, 1], "w = 1");
    assert_eq!(window_maxima(&[7, 1], 2), vec![7], "window is the whole slice");
    assert_eq!(window_maxima(&[7, 1], 3), Vec::<i32>::new(), "window too big");
    assert_eq!(window_maxima(&[7, 1], 0), Vec::<i32>::new(), "w = 0");
    assert_eq!(window_maxima(&[-5, -1, -7, -3], 2), vec![-1, -1, -3], "negatives");
}
