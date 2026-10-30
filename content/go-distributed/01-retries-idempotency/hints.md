Server state per client: the last `Seq` you applied and the `Response` you produced for it. A `map[string]entry` is enough.
===
Three cases in `Handle`: `Seq == last+1` apply and store; `Seq == last` return the stored reply; anything else leave the balance alone.
===
The client needs only `acked int`. `Pending()` builds `Request{id, acked+1, amounts[acked]}` while `acked < len(amounts)`.
===
In `OnResponse`, compare both `r.Client == id` and `r.Seq == acked+1` before incrementing. A late reply to an earlier request has a smaller `Seq`.
