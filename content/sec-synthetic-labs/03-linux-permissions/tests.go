package main

import (
	"archive/tar"
	"bytes"
	"fmt"
	"reflect"
	"testing"
)

func TestTarVariants(t *testing.T) {
	for i := 0; i < 16; i++ {
		var b bytes.Buffer
		w := tar.NewWriter(&b)
		name := fmt.Sprintf("opt/app%d.key", i)
		headers := []tar.Header{{Name: name, Mode: 04666, Uid: 0, Typeflag: tar.TypeReg}, {Name: "bin/tool", Mode: 04755, Uid: 0, Typeflag: tar.TypeReg}, {Name: "bin/user", Mode: 04755, Uid: 1000, Typeflag: tar.TypeReg}, {Name: "tmp/", Mode: 0777, Typeflag: tar.TypeDir}, {Name: "etc/shadow", Mode: 0600, Typeflag: tar.TypeReg}, {Name: "link.key", Mode: 0777, Typeflag: tar.TypeSymlink, Linkname: "safe"}}
		for _, h := range headers {
			if err := w.WriteHeader(&h); err != nil {
				t.Fatal(err)
			}
		}
		if err := w.Close(); err != nil {
			t.Fatal(err)
		}
		got, err := AuditTar(b.Bytes())
		want := []Finding{{"bin/tool", "root-suid"}, {name, "exposed-secret"}, {name, "world-writable"}}
		if err != nil || !reflect.DeepEqual(got, want) {
			t.Fatalf("got %+v %v want %+v", got, err, want)
		}
	}
}
func TestTarCorruption(t *testing.T) {
	if _, err := AuditTar([]byte("not a tar file")); err == nil {
		t.Fatal("corrupt input accepted")
	}
}
func TestSafeTar(t *testing.T) {
	var b bytes.Buffer
	w := tar.NewWriter(&b)
	w.WriteHeader(&tar.Header{Name: "safe", Mode: 0600, Typeflag: tar.TypeReg})
	w.Close()
	got, err := AuditTar(b.Bytes())
	if err != nil || len(got) != 0 {
		t.Fatal(got, err)
	}
}

func TestTarPermissionMatrix(t *testing.T) {
	for i, mode := range []int64{0000, 0002, 0004, 0040, 0044, 0600, 0644, 0666, 04000, 04001, 04010, 04100, 04777} {
		for _, uid := range []int{0, 1000} {
			var b bytes.Buffer
			w := tar.NewWriter(&b)
			name := fmt.Sprintf("etc/secret%d.key", i)
			if err := w.WriteHeader(&tar.Header{Name: name, Mode: mode, Uid: uid, Typeflag: tar.TypeReg}); err != nil {
				t.Fatal(err)
			}
			w.Close()
			got, err := AuditTar(b.Bytes())
			want := []Finding{}
			if mode&0044 != 0 {
				want = append(want, Finding{name, "exposed-secret"})
			}
			if uid == 0 && mode&04000 != 0 && mode&0111 != 0 {
				want = append(want, Finding{name, "root-suid"})
			}
			if mode&0002 != 0 {
				want = append(want, Finding{name, "world-writable"})
			}
			if err != nil || !reflect.DeepEqual(got, want) {
				t.Fatalf("mode=%o uid=%d: got %v %v want %v", mode, uid, got, err, want)
			}
		}
	}
}
