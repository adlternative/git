
#!/bin/sh

test_description='Basic sort performance tests'
. ./perf-lib.sh

test_perf_default_repo

test_perf 'for-each-ref with default format' '
	git for-each-ref
'


test_perf 'for-each-ref with refname' '
	git for-each-ref --format="%(refname)"
'

test_perf 'for-each-ref with objectname' '
	git for-each-ref --format="%(objectname)"
'

test_perf 'for-each-ref with objecttype' '
	git for-each-ref --format="%(objecttype)"
'

test_perf 'for-each-ref with objectsize' '
	git for-each-ref --format="%(objectsize)"
'

test_perf 'for-each-ref with objectsize:disk' '
	git for-each-ref --format="%(objectsize:disk)"
'

test_perf 'for-each-ref with deltabase' '
	git for-each-ref --format="%(deltabase)"
'

test_perf 'for-each-ref with tree' '
	git for-each-ref --format="%(tree)"
'

test_perf 'for-each-ref with parent' '
	git for-each-ref --format="%(parent)"
'

test_perf 'for-each-ref with numparent' '
	git for-each-ref --format="%(numparent)"
'

test_perf 'for-each-ref with object' '
	git for-each-ref --format="%(object)"
'

test_perf 'for-each-ref with type' '
	git for-each-ref --format="%(type)"
'

test_perf 'for-each-ref with tag' '
	git for-each-ref --format="%(tag)"
'

test_perf 'for-each-ref with author' '
	git for-each-ref --format="%(author)"
'

test_perf 'for-each-ref with authorname' '
	git for-each-ref --format="%(authorname)"
'

test_perf 'for-each-ref with authoremail' '
	git for-each-ref --format="%(authoremail)"
'

test_perf 'for-each-ref with authordate' '
	git for-each-ref --format="%(authordate)"
'

test_perf 'for-each-ref with committer' '
	git for-each-ref --format="%(committer)"
'

test_perf 'for-each-ref with committername' '
	git for-each-ref --format="%(committername)"
'

test_perf 'for-each-ref with committeremail' '
	git for-each-ref --format="%(committeremail)"
'

test_perf 'for-each-ref with committerdate' '
	git for-each-ref --format="%(committerdate)"
'

test_perf 'for-each-ref with tagger' '
	git for-each-ref --format="%(tagger)"
'

test_perf 'for-each-ref with taggername' '
	git for-each-ref --format="%(taggername)"
'

test_perf 'for-each-ref with taggeremail' '
	git for-each-ref --format="%(taggeremail)"
'

test_perf 'for-each-ref with taggerdate' '
	git for-each-ref --format="%(taggerdate)"
'

test_perf 'for-each-ref with creator' '
	git for-each-ref --format="%(creator)"
'

test_perf 'for-each-ref with creatordate' '
	git for-each-ref --format="%(creatordate)"
'

test_perf 'for-each-ref with subject' '
	git for-each-ref --format="%(subject)"
'

test_perf 'for-each-ref with body' '
	git for-each-ref --format="%(body)"
'

test_perf 'for-each-ref with trailers' '
	git for-each-ref --format="%(trailers)"
'

test_perf 'for-each-ref with contents' '
	git for-each-ref --format="%(contents)"
'

test_perf 'for-each-ref with upstream' '
	git for-each-ref --format="%(upstream)"
'

test_perf 'for-each-ref with push' '
	git for-each-ref --format="%(push)"
'

test_perf 'for-each-ref with symref' '
	git for-each-ref --format="%(symref)"
'

test_perf 'for-each-ref with flag' '
	git for-each-ref --format="%(flag)"
'

test_perf 'for-each-ref with HEAD' '
	git for-each-ref --format="%(HEAD)"
'

test_perf 'for-each-ref with worktreepath' '
	git for-each-ref --format="%(worktreepath)"
'

test_done
