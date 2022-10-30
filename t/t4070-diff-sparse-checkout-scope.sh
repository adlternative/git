#!/bin/sh

test_description='diff sparse-checkout scope'

GIT_TEST_DEFAULT_INITIAL_BRANCH_NAME=main
export GIT_TEST_DEFAULT_INITIAL_BRANCH_NAME

. ./test-lib.sh


test_expect_success 'setup' '
	git init temp &&
	(
		cd temp &&
		mkdir sub1 &&
		mkdir sub2 &&
		echo sub1/file1 >sub1/file1 &&
		echo sub2/file2 >sub2/file2 &&
		echo file1 >file1 &&
		echo file2 >file2 &&
		git add --all &&
		git commit -m init &&
		echo sub1/file1 >>sub1/file1 &&
		echo sub1/file2 >>sub1/file2 &&
		echo sub2/file1 >>sub2/file1 &&
		echo sub2/file2 >>sub2/file2 &&
		echo file1 >>file1 &&
		echo file2 >>file2 &&
		git add --all &&
		git commit -m change1 &&
		echo sub1/file1 >>sub1/file1 &&
		echo sub1/file2 >>sub1/file2 &&
		echo sub2/file1 >>sub2/file1 &&
		echo sub2/file2 >>sub2/file2 &&
		echo file1 >>file1 &&
		echo file2 >>file2 &&
		git add --all &&
		git commit -m change2
	)
'

reset_repo () {
	rm -rf repo &&
	git clone --no-checkout temp repo
}

reset_with_sparse_checkout() {
	reset_repo &&
	git -C repo sparse-checkout set --$1 sub1 &&
	git -C repo checkout
}

test_expect_success 'builtin_diff_tree no-cone, sparse scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --scope=sparse HEAD HEAD~ >actual &&
	cat > expect <<-EOF &&
sub1/file1
sub1/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_tree cone, sparse scope' '
	reset_with_sparse_checkout cone &&
	git -C repo diff --name-only --scope=sparse HEAD HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_tree no-cone, all scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --scope=all HEAD HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
sub2/file1
sub2/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_tree cone, all scope' '
	reset_with_sparse_checkout cone &&
	git -C repo diff --name-only --scope=all HEAD HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
sub2/file1
sub2/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_tree no-cone, no scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --no-scope HEAD HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
sub2/file1
sub2/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_tree cone, no scope' '
	reset_with_sparse_checkout cone &&
	git -C repo diff --name-only --no-scope HEAD HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
sub2/file1
sub2/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_index no-cone, sparse scope' '
	reset_with_sparse_checkout no-cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=sparse HEAD~ >actual &&
	cat > expect <<-EOF &&
sub1/file1
sub1/file2
sub1/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_index cone, sparse scope' '
	reset_with_sparse_checkout cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=sparse HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
file3
sub1/file1
sub1/file2
sub1/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_index no-cone, all scope' '
	reset_with_sparse_checkout no-cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=all HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
file3
sub1/file1
sub1/file2
sub1/file3
sub2/file1
sub2/file2
sub2/file3
sub3/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_index cone, all scope' '
	reset_with_sparse_checkout cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=all HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
file3
sub1/file1
sub1/file2
sub1/file3
sub2/file1
sub2/file2
sub2/file3
sub3/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_index no-cone, no scope' '
	reset_with_sparse_checkout no-cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --no-scope HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
file3
sub1/file1
sub1/file2
sub1/file3
sub2/file1
sub2/file2
sub2/file3
sub3/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_index cone, no scope' '
	reset_with_sparse_checkout cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --no-scope HEAD~ >actual &&
	cat > expect <<-EOF &&
file1
file2
file3
sub1/file1
sub1/file2
sub1/file3
sub2/file1
sub2/file2
sub2/file3
sub3/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'diff_no_index cone, sparse scope' '
	reset_with_sparse_checkout cone &&
	test_expect_code 1 git -C repo diff --no-index --name-only --scope=sparse file1 sub1/file1 >actual &&
	cat > expect <<-EOF &&
sub1/file1
	EOF
	test_cmp expect actual &&
	test_expect_code 1 git -C repo diff --no-index --name-only --scope=sparse sub1/file1 sub2/file2 >actual &&
	echo -n >expect &&
	test_cmp expect actual
'

test_expect_success 'diff_no_index no-cone, sparse scope' '
	reset_with_sparse_checkout no-cone &&
	test_expect_code 1 git -C repo diff --no-index --name-only --scope=sparse sub1/file1 sub1/file2 >actual &&
	cat > expect <<-EOF &&
sub1/file2
	EOF
	test_cmp expect actual &&
	test_expect_code 1 git -C repo diff --no-index --name-only --scope=sparse file1 sub1/file1 >actual &&
	echo -n >expect &&
	test_cmp expect actual
'

test_expect_success 'diff_no_index cone, all scope' '
	reset_with_sparse_checkout cone &&
	test_expect_code 1 git -C repo diff --no-index --name-only --scope=all file1 sub1/file1 >actual &&
	cat > expect <<-EOF &&
sub1/file1
	EOF
	test_cmp expect actual &&
	test_expect_code 1 git -C repo diff --no-index --name-only --scope=all sub1/file1 sub2/file2 >actual &&
	echo -n >expect &&
	test_cmp expect actual
'

test_expect_success 'diff_no_index no-cone, all scope' '
	reset_with_sparse_checkout no-cone &&
	test_expect_code 1 git -C repo diff --no-index --name-only --scope=all sub1/file1 sub1/file2 >actual &&
	cat > expect <<-EOF &&
sub1/file2
	EOF
	test_cmp expect actual &&
	test_expect_code 1 git -C repo diff --no-index --name-only --scope=all file1 sub1/file1 >actual &&
	echo -n >expect &&
	test_cmp expect actual
'

test_expect_success 'diff_no_index cone, no scope' '
	reset_with_sparse_checkout cone &&
	test_expect_code 1 git -C repo diff --no-index --name-only --no-scope file1 sub1/file1 >actual &&
	cat > expect <<-EOF &&
sub1/file1
	EOF
	test_cmp expect actual &&
	test_expect_code 1 git -C repo diff --no-index --name-only --no-scope sub1/file1 sub2/file2 >actual &&
	echo -n >expect &&
	test_cmp expect actual
'

test_expect_success 'diff_no_index no-cone, no scope' '
	reset_with_sparse_checkout no-cone &&
	test_expect_code 1 git -C repo diff --no-index --name-only --no-scope sub1/file1 sub1/file2 >actual &&
	cat > expect <<-EOF &&
sub1/file2
	EOF
	test_cmp expect actual &&
	test_expect_code 1 git -C repo diff --no-index --name-only --no-scope file1 sub1/file1 >actual &&
	echo -n >expect &&
	test_cmp expect actual
'

test_expect_success 'builtin_diff_files cone, sparse scope' '
	reset_with_sparse_checkout cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=sparse file3 sub1/ sub2/ >actual &&
	cat > expect <<-EOF &&
file3
sub1/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_files no-cone, sparse scope' '
	reset_with_sparse_checkout no-cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=sparse file3 sub1/ sub2/ >actual &&
	cat > expect <<-EOF &&
sub1/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_files cone, all scope' '
	reset_with_sparse_checkout cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=all file3 sub1/ sub2/ >actual &&
	cat > expect <<-EOF &&
file3
sub1/file3
sub2/file1
sub2/file2
sub2/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_files no-cone, all scope' '
	reset_with_sparse_checkout no-cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=all file3 sub1/ sub2/ >actual &&
	cat > expect <<-EOF &&
file3
sub1/file3
sub2/file1
sub2/file2
sub2/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_files cone, no scope' '
	reset_with_sparse_checkout cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --no-scope file3 sub1/ sub2/ >actual &&
	cat > expect <<-EOF &&
file3
sub1/file3
sub2/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_files no-cone, no scope' '
	reset_with_sparse_checkout no-cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --no-scope file3 sub1/ sub2/ >actual &&
	cat > expect <<-EOF &&
file3
sub1/file3
sub2/file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_b_f cone, sparse scope' '
	reset_with_sparse_checkout cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=sparse HEAD~:sub1/file1 file3 >actual &&
	cat > expect <<-EOF &&
file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_b_f no-cone, sparse scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --scope=sparse HEAD~:sub2/file2 sub1/file2 >actual &&
	echo -n >expect &&
	test_cmp expect actual
'

test_expect_success 'builtin_diff_b_f cone, all scope' '
	reset_with_sparse_checkout cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --scope=all HEAD~:sub1/file1 file3 >actual &&
	cat > expect <<-EOF &&
file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_b_f no-cone, all scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --scope=all HEAD~:sub2/file2 sub1/file2 >actual &&
	cat > expect <<-EOF &&
sub1/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_b_f cone, no scope' '
	reset_with_sparse_checkout cone &&
	(
		cd repo &&
		mkdir sub2 sub3 &&
		echo sub1/file3 >sub1/file3 &&
		echo sub2/file3 >sub2/file3 &&
		echo sub3/file3 >sub3/file3 &&
		echo file3 >file3 &&
		git add --all --sparse &&
		echo sub1/file3 >>sub1/file3 &&
		echo sub2/file3 >>sub2/file3 &&
		echo sub3/file3 >>sub3/file3 &&
		echo file3 >>file3
	) &&
	git -C repo diff --name-only --no-scope HEAD~:sub1/file1 file3 >actual &&
	cat > expect <<-EOF &&
file3
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_b_f no-cone, no scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --no-scope HEAD~:sub2/file2 sub1/file2 >actual &&
	cat > expect <<-EOF &&
sub1/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_blobs cone, sparse scope' '
	reset_with_sparse_checkout cone &&
	git -C repo diff --name-only --scope=sparse HEAD~:sub1/file1 HEAD:file2 >actual &&
	cat > expect <<-EOF &&
file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_blobs no-cone, sparse scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --scope=sparse HEAD~:sub2/file2 HEAD:sub1/file2 >actual &&
	echo -n >expect &&
	test_cmp expect actual
'

test_expect_success 'builtin_diff_blobs cone, all scope' '
	reset_with_sparse_checkout cone &&
	git -C repo diff --name-only --scope=all HEAD~:sub1/file1 HEAD:file2 >actual &&
	cat > expect <<-EOF &&
file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_blobs no-cone, all scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --scope=all HEAD~:sub2/file2 HEAD:sub1/file2 >actual &&
	cat > expect <<-EOF &&
sub1/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_blobs cone, no scope' '
	reset_with_sparse_checkout cone &&
	git -C repo diff --name-only --no-scope HEAD~:sub1/file1 HEAD:file2 >actual &&
	cat > expect <<-EOF &&
file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_blobs no-cone, no scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --no-scope HEAD~:sub2/file2 HEAD:sub1/file2 >actual &&
	cat > expect <<-EOF &&
sub1/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_combined cone, sparse scope' '
	reset_with_sparse_checkout cone &&
	git -C repo diff --name-only --scope=sparse HEAD~2 HEAD~ HEAD >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_combined no-cone, sparse scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --scope=sparse HEAD~2 HEAD~ HEAD >actual &&
	cat > expect <<-EOF &&
sub1/file1
sub1/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_combined cone, all scope' '
	reset_with_sparse_checkout cone &&
	git -C repo diff --name-only --scope=all HEAD~2 HEAD~ HEAD >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
sub2/file1
sub2/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_combined no-cone, all scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --scope=all HEAD~2 HEAD~ HEAD >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
sub2/file1
sub2/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_combined cone, no scope' '
	reset_with_sparse_checkout cone &&
	git -C repo diff --name-only --no-scope HEAD~2 HEAD~ HEAD >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
sub2/file1
sub2/file2
	EOF
	test_cmp expect actual
'

test_expect_success 'builtin_diff_combined no-cone, sparse scope' '
	reset_with_sparse_checkout no-cone &&
	git -C repo diff --name-only --no-scope HEAD~2 HEAD~ HEAD >actual &&
	cat > expect <<-EOF &&
file1
file2
sub1/file1
sub1/file2
sub2/file1
sub2/file2
	EOF
	test_cmp expect actual
'
# no scope and config and sparse-index...

test_done
