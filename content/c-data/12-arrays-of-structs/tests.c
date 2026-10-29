int main(void) {
    Entry b[5] = {{1, 70}, {2, 95}, {3, 80}, {4, 95}, {5, 60}};
    CHECK("best_index finds the highest score", best_index(b, 5) == 1, "got %d", best_index(b, 5));
    Entry c[3] = {{7, 10}, {8, 20}, {9, 30}};
    CHECK("best_index with the best last", best_index(c, 3) == 2, "got %d", best_index(c, 3));
    Entry d[3] = {{7, -5}, {8, -20}, {9, -9}};
    CHECK("best_index with negative scores", best_index(d, 3) == 0, "got %d", best_index(d, 3));
    CHECK("best_index of a single record is 0", best_index(c, 1) == 0, "got %d", best_index(c, 1));
    CHECK("best_index respects n", best_index(b, 1) == 0, "got %d", best_index(b, 1));

    CHECK("total_score adds all five", total_score(b, 5) == 400, "got %d", total_score(b, 5));
    CHECK("total_score of an empty table is 0", total_score(b, 0) == 0, "got %d", total_score(b, 0));
    CHECK("total_score respects n", total_score(c, 2) == 30, "got %d", total_score(c, 2));

    CHECK("count_passing at 80 counts three", count_passing(b, 5, 80) == 3, "got %d", count_passing(b, 5, 80));
    CHECK("count_passing is inclusive", count_passing(b, 5, 95) == 2, "got %d", count_passing(b, 5, 95));
    CHECK("count_passing above everything is 0", count_passing(b, 5, 96) == 0, "got %d", count_passing(b, 5, 96));
    CHECK("count_passing with a low mark counts all", count_passing(d, 3, -100) == 3, "got %d", count_passing(d, 3, -100));

    Entry e[3] = {{1, 10}, {2, 20}, {3, 30}};
    add_bonus(e, 3, 5);
    CHECK("add_bonus raises every score", e[0].score == 15 && e[1].score == 25 && e[2].score == 35, "got %d %d %d", e[0].score, e[1].score, e[2].score);
    CHECK("add_bonus leaves ids alone", e[0].id == 1 && e[1].id == 2 && e[2].id == 3, "ids changed");
    add_bonus(e, 2, -15);
    CHECK("add_bonus respects n and allows negatives", e[0].score == 0 && e[1].score == 10 && e[2].score == 35, "got %d %d %d", e[0].score, e[1].score, e[2].score);

    sort_by_score(b, 5);
    int ids[5] = {2, 4, 3, 1, 5};
    int ok = 1;
    for (int i = 0; i < 5; i++) if (b[i].id != ids[i]) ok = 0;
    CHECK("sort_by_score orders high to low and keeps ties stable", ok, "got ids %d %d %d %d %d", b[0].id, b[1].id, b[2].id, b[3].id, b[4].id);
    CHECK("sort_by_score keeps each score with its id", b[0].score == 95 && b[2].score == 80 && b[4].score == 60, "scores got separated from ids");
    Entry f[6] = {{1, 3}, {2, 3}, {3, 3}, {4, 1}, {5, 3}, {6, 2}};
    sort_by_score(f, 6);
    int fid[6] = {1, 2, 3, 5, 6, 4};
    ok = 1;
    for (int i = 0; i < 6; i++) if (f[i].id != fid[i]) ok = 0;
    CHECK("sort_by_score is stable with many ties", ok, "got ids %d %d %d %d %d %d", f[0].id, f[1].id, f[2].id, f[3].id, f[4].id, f[5].id);
    Entry g[4] = {{1, 1}, {2, 2}, {3, 3}, {4, 4}};
    sort_by_score(g, 2);
    CHECK("sort_by_score respects n", g[0].id == 2 && g[1].id == 1 && g[2].id == 3 && g[3].id == 4, "got ids %d %d %d %d", g[0].id, g[1].id, g[2].id, g[3].id);
    sort_by_score(g, 0);
    CHECK("sort_by_score of an empty table is safe", g[0].id == 2, "table changed");
    return 0;
}
