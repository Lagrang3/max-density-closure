import io
import copy
import random
import subprocess

random.seed(25)

class tx:
    def __init__(self, idx, fee, size, depends=[]):
        self.idx = idx
        self.size = size
        self.fee = fee
        self.depends = copy.deepcopy(depends)

class tcase:
    def __init__(self, txs = []):
        self.txs = copy.deepcopy(txs)
    
    def dependencies(self):
        r = []
        for tx in self.txs:
            for d in tx.depends:
                r.append((tx.idx, d))
        return r
    
    def write(self, fd):
        N = len(self.txs)
        deps = self.dependencies()
        M = len(deps)
        print(N,M,file=fd)
        for tx in self.txs:
            print(tx.fee, tx.size, file=fd)
        for d in deps:
            print(d[0], d[1], file=fd)

def random_topology(rates, m):
    txs = [ tx(i, r[0], r[1]) for i,r in enumerate(rates) ]
    n = len(txs)
    set_value = [ i for i in range(n)]
    compact_depends = [ set([i]) for i in range(n) ]

    def set_of(i):
        r = i
        while set_value[r]!=r:
            new_r = set_value[r]
            set_value[r] = new_r
            r = new_r
        return r
    def merge_set(i, j):
        i = set_of(i)
        j = set_of(j)
        if i!=j:
            set_value[i] = j
    def depends(i, j):
        return j in compact_depends[i]
    def add_arc(i, j):
        # print(f"add arc {i} {j}")
        merge_set(i, j)
        assert not depends(j, i)
        txs[i].depends.append(j)
        # i and all i's ancestors now depend on j and all j's depedency set
        compact_depends[i] = compact_depends[i].union(compact_depends[j])
        for x in range(n):
            if depends(x, i):
                compact_depends[x] = compact_depends[x].union(compact_depends[j])

    n_arcs = n-1
    assert m>=n_arcs
    while n_arcs>0:
        a = random.randint(0, n-1)
        b = random.randint(0, n-1)
        if set_of(a)==set_of(b):
            continue
        n_arcs -= 1
        m -= 1
        add_arc(a,b)
    while m>0:
        a = random.randint(0, n-1)
        b = random.randint(0, n-1)
        if depends(b, a):
            continue
        m -= 1
        add_arc(a,b)
    return tcase(txs)

def test_and_report(test_case):
    print("===================")
    timeout = 2
    test_exec = "../build/examples/maxfeerate-fp"
    test_in = io.StringIO()
    test_case.write(test_in)
    print("Input:")
    print(test_in.getvalue())
    try:
        test = subprocess.Popen([test_exec], stdin=subprocess.PIPE,
            stdout = subprocess.PIPE,
            stderr = subprocess.PIPE)
        test_out = test.communicate(input=test_in.getvalue().encode('utf-8'),timeout=timeout)
        test.wait()
        if test.returncode!=0:
            raise subprocess.CalledProcessError(test.returncode, test.args)
    except subprocess.TimeoutExpired as e:
        print("Time Limit Exceeded:", e)
        return
    except subprocess.CalledProcessError as e:
        print("Runtime error: ", e)
        return
    except Exception as e:
        print("An error occurred: ", e)
        return
    print("Output:")
    print(test_out[0].decode())
    
    validate_exec = "../build/examples/maxfeerate-validate"
    try:
        validate = subprocess.Popen([validate_exec], stdin=subprocess.PIPE,
            stdout = subprocess.PIPE,
            stderr = subprocess.PIPE)
        validate_out = validate.communicate(
            input=test_in.getvalue().encode('utf-8')+test_out[0])
        validate.wait()
        if validate.returncode!=0:
            raise subprocess.CalledProcessError(test.returncode, test.args)
    except subprocess.CalledProcessError as e:
        print("Wrong Answer")
        return
    print("Accepted")


test_cases = []
test_cases.append(tcase([]))
test_cases.append(tcase([tx(0,1,1)]))
test_cases.append(tcase([tx(0,1,2),tx(1,2,4)]))
test_cases.append(tcase([tx(0,2,4),tx(1,1,3)]))
test_cases.append(tcase([tx(0,2,4),tx(1,1,3,[0])]))
test_cases.append(tcase([tx(0,2,4),tx(1,1,3,[0])]))
test_cases.append(tcase([tx(0,2,4,[1]),tx(1,1,3)]))
test_cases.append(tcase([tx(0,2,4,[1,2]),tx(1,1,3),tx(2,4,7)]))
test_cases.append(tcase([tx(0,2,4,[1,2]),tx(1,1,3),tx(2,4,7),tx(3,8,10,[0])]))
test_cases.append(random_topology([(1,1), (1,1), (2,3), (2,1), (3,4)], 7))

if __name__=="__main__":
    for t in test_cases:
        test_and_report(t)
