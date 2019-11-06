import os
import re
import data
import fi_tools
import sys

def results(resdir, tool, config, wait, app, action, instrument, nthreads, inputsize, start, end, verbose):
    print('===== APP %s %s %s %s %s %s %s %s %s  ====='%(app, tool, config, action, instrument, nthreads, inputsize, start, end) )
    missing = timeout = crash = soc = benign = 0
    # base app dir
    rootdir = '%s/%s/%s/%s/'%(resdir, 'golden', config, wait)
    # specific trial dir
    profiledir = '%s/%s/profile/%s/%s/'%(rootdir, app, nthreads, inputsize )

    # Use the first experiment on golden to get the valid output
    # TODO: check whether outputs with omp may differ but still valid
    trialdir = profiledir + '1/'

    verify_list = []
    with open(trialdir + 'output.txt', 'r') as f:
        for v in data.programs[config][app]['verify'][inputsize]:
            #print(v)
            m = re.findall(v, f.read())
            verify_list += m
    #print(verify_list)
    assert verify_list, 'verify_list cannot be empty file: ' + trialdir + 'output.txt' + ', verify:' + ' '.join(data.programs[config][app]['verify'][inputsize])

    basedir = '%s/%s/%s/%s/%s/%s/%s/%s/%s/'%(resdir, tool, config, wait, app, action, instrument, nthreads, inputsize)

    for trial in range(start, end+1):
        trialdir = '%s/%s/'%( basedir, trial )
        if not os.path.isfile(trialdir + '/ret.txt'):
            print('PENDING trial %d'%( trial ) )
            continue

        #print('trial %s' % ( trialdir ) )
        #sys.exit(123) #ggout
        with open(trialdir + 'ret.txt', 'r') as retf:
            if not os.path.isfile(trialdir + fi_tools.files[tool]['injection']):
                missing += 1
            else:
                res = retf.read()
                res = res.strip().split(',')
                if res[0] == 'timeout':
                    timeout += 1
                elif res[0] == 'crash':
                    crash += 1
                elif res[0] == 'error':
                    crash += 1
                elif res[0] == 'exit':
                    with open(trialdir + '/' + 'output.txt', 'r') as f:
                        #print('open: ' + trialdir +'/' + 'output.txt')
                        verify_out = []
                        for v in data.programs[config][app]['verify'][inputsize]:
                            m = re.findall(v, f.read())
                            verify_out += m
                        verified = True
                        for out in verify_list:
                            if not out in verify_out:
                                verified = False
                                break
                        if verbose:
                            print('*** verify ***')
                            print(verify_list)
                            print(verify_out)
                            print('trial %d verified: %s'%(trial, verified))
                            print('*** end verify ***')
                        if verified:
                            benign += 1
                        else:
                            soc += 1
                            #TODO: extend verifier with tolerance
                        #print('\ttimeout: ' + str(timeout) + ', crash: ' + str(crash) + ', soc: ' + str(soc) + ', benign: ' + str(benign))
                else:
                    print('Invalid result ' + tool + ' ' + app + ' ' + str(trial) +' :' + str(res[0]))

    print('\tmissing: ' + str(missing) + ', timeout: ' + str(timeout) + ', crash: ' + str(crash) + ', soc: ' + str(soc) + ', benign: ' + str(benign))
    total = end - start + 1
    print('\t(%%) timeout %3.2f, crash %3.2f, soc %3.2f, benign %3.2f'%(timeout*100.0/total, crash*100.0/total, soc*100.0/total, benign*100.0/total) )
    print('===== END APP ' + app +' =====\n');
    if config == 'serial':
        with open('results-%s-%s-serial-%s-%s-%s.txt'%( app, tool, inputsize, start, end), 'w' ) as f:
            f.write('missing: %d\n'%(missing) )
            f.write('timeout: %d\n'%(timeout) )
            f.write('crash: %d\n'%( crash ) )
            f.write('soc: %d\n'%( soc ) )
            f.write('benign: %d\n'%( benign ) )
    else: # omp
        with open('results-%s-%s-omp-%s-%s-%s-%s-%s.txt'%( app, tool, inputsize, nthreads, instrument, start, end), 'w') as f:
            f.write('missing: %d\n'%(missing) )
            f.write('timeout: %d\n'%(timeout) )
            f.write('crash: %d\n'%( crash ) )
            f.write('soc: %d\n'%( soc ) )
            f.write('benign: %d\n'%( benign ) )

