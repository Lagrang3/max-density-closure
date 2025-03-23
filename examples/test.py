import io
import subprocess

class tx:
    def __init__(self, idx, fee, size, depends=[]):
        self.idx = idx
        self.size = size
        self.fee = fee
        self.depends = depends

class tcase:
    def __init__(self, txs = []):
        self.txs = txs
    
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


def test_and_report(test_case):
    print("===================")
    timeout = 2
    test_exec = "../build/examples/maxfeerate-ggt"
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

if __name__=="__main__":
    for t in test_cases:
        test_and_report(t)
