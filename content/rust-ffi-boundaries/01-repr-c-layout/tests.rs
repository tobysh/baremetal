fn lcg(s: &mut u64) -> u64 {
    *s = s.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
    *s >> 33
}

#[test]
fn wide_layout_is_measured() {
    // Worked out by hand: a@0, pad 3, b@4, c@8, pad 1, d@10, size 12.
    assert_eq!(wide_report(), Report { size: 12, align: 4, off_b: 4, off_c: 8, off_d: 10 });
}

#[test]
fn tight_is_eight_bytes() {
    assert_eq!(std::mem::size_of::<Tight>(), 8, "reorder the fields to remove padding");
    assert_eq!(std::mem::align_of::<Tight>(), 4);
    let r = tight_report();
    assert_eq!(r.size, 8);
    assert_eq!(r.align, 4);
    assert_eq!(r.off_b % 4, 0, "u32 must be 4-aligned");
    assert_eq!(r.off_d % 2, 0, "u16 must be 2-aligned");
    assert_eq!(r.off_b, std::mem::offset_of!(Tight, b));
    assert_eq!(r.off_c, std::mem::offset_of!(Tight, c));
    assert_eq!(r.off_d, std::mem::offset_of!(Tight, d));
}

#[test]
fn tight_fields_do_not_overlap() {
    let spans = [
        (std::mem::offset_of!(Tight, a), 1),
        (std::mem::offset_of!(Tight, b), 4),
        (std::mem::offset_of!(Tight, c), 1),
        (std::mem::offset_of!(Tight, d), 2),
    ];
    let mut used = [false; 8];
    for (o, n) in spans {
        for i in o..o + n {
            assert!(!used[i], "byte {i} used twice");
            used[i] = true;
        }
    }
    assert!(used.iter().all(|&u| u), "tight layout should leave no gaps");
}

#[test]
fn widen_copies_every_field() {
    let mut s = 7u64;
    for _ in 0..50 {
        let (a, b, c, d) = (lcg(&mut s) as u8, lcg(&mut s) as u32, lcg(&mut s) as u8, lcg(&mut s) as u16);
        let t = Tight { a, b, c, d };
        assert_eq!(widen(&t), Wide { a, b, c, d });
    }
}

#[test]
fn bytes_match_a_hand_written_layout() {
    let w = Wide { a: 0xAA, b: 0x11223344, c: 0xBB, d: 0x5566 };
    assert_eq!(
        to_bytes(&w),
        [0xAA, 0, 0, 0, 0x44, 0x33, 0x22, 0x11, 0xBB, 0, 0x66, 0x55],
        "a, 3 zero pad, b little-endian, c, 1 zero pad, d little-endian"
    );
    assert_eq!(to_bytes(&Wide { a: 0, b: 0, c: 0, d: 0 }), [0; 12]);
    assert_eq!(to_bytes(&Wide { a: 255, b: u32::MAX, c: 255, d: u16::MAX }), [255, 0, 0, 0, 255, 255, 255, 255, 255, 0, 255, 255]);
}

#[test]
fn bytes_for_many_values() {
    let mut s = 99u64;
    for _ in 0..100 {
        let (a, b, c, d) = (lcg(&mut s) as u8, lcg(&mut s) as u32, lcg(&mut s) as u8, lcg(&mut s) as u16);
        let bb = b.to_le_bytes();
        let dd = d.to_le_bytes();
        let want = [a, 0, 0, 0, bb[0], bb[1], bb[2], bb[3], c, 0, dd[0], dd[1]];
        assert_eq!(to_bytes(&Wide { a, b, c, d }), want);
    }
}
