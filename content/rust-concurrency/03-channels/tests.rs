#[test]
fn fan_in_collects_everything_sorted() {
    let r = fan_in(3, 2);
    assert_eq!(r, vec![0, 1, 1000, 1001, 2000, 2001], "all six messages, sorted");
}

#[test]
fn fan_in_larger() {
    let r = fan_in(5, 100);
    assert_eq!(r.len(), 500, "5 producers x 100 messages should give 500 values");
    assert_eq!(r[0], 0);
    assert_eq!(r[499], 4099, "the last value should be 4*1000+99");
    assert!(r.windows(2).all(|w| w[0] < w[1]), "values should be sorted and unique");
}

#[test]
fn fan_in_no_work_terminates() {
    assert!(fan_in(0, 5).is_empty(), "no producers means no messages (and the loop must still end)");
    assert!(fan_in(4, 0).is_empty(), "producers that send nothing still must not hang the receiver");
}

#[test]
fn pipeline_transforms_in_order() {
    assert_eq!(pipeline(vec![1, 2, 3, -4]), vec![3, 5, 7, -7], "each n becomes n*2+1, order kept");
}

#[test]
fn pipeline_empty_and_large() {
    assert!(pipeline(vec![]).is_empty(), "empty in, empty out");
    let input: Vec<i32> = (0..1000).collect();
    let out = pipeline(input);
    assert_eq!(out.len(), 1000, "all replies must arrive");
    assert_eq!(out[999], 1999, "last reply should be 999*2+1");
}

#[test]
fn drain_ready_is_non_blocking() {
    let (tx, rx) = mpsc::channel();
    assert!(drain_ready(&rx).is_empty(), "an empty channel drains to an empty Vec without blocking");
    tx.send(1).unwrap();
    tx.send(2).unwrap();
    tx.send(3).unwrap();
    assert_eq!(drain_ready(&rx), vec![1, 2, 3], "three waiting messages");
    assert!(drain_ready(&rx).is_empty(), "already drained");
    tx.send(9).unwrap();
    assert_eq!(drain_ready(&rx), vec![9], "later messages can be drained again");
}
