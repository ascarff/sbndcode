'''Generate the GDML for the SBND CRT geometry, Bottom Pit Edition.

CRT arrangement in the pit bottom, see SBN Document 1533-v2.

We define a global XML DOM which the function for each volume will append to.
Volume-building functions are called hierarchically, so if you call module(),
it will construct the module and all the parts that make it up, so you end
up with complete GDML for one module.

Each physical volume has a corresponding unique logical volume, as required
by LArG4 to keep track of energy depositions. The solids, however, can safely
be referenced many times, and so are stored only once (using a hash keyed on
the the linear dimensions).

The output of this code is a file "crt.gdml" which contains the GDML snippets
to paste into the full SBND geometry.

A. Mastbaum <mastbaum@uchicago.edu>, 2016/10/27
'''

import xml.etree.cElementTree as ET
from xml.dom import minidom
from hashlib import md5

# Options
PAD = 0.4  # Padding between strips and module (Al thickness)

##########

gdml = ET.Element('gdml')
solids = ET.SubElement(gdml, 'solids')
structure = ET.SubElement(gdml, 'structure')
solids_store = {}

seq_id = 0

def sequential_id():
    global seq_id
    s = str(seq_id)
    seq_id += 1
    return s


def strip(x, y, z=1.0):
    '''Build one scintillator strip.'''
    xx = str(x)
    yy = str(y - 2 * PAD)
    zz = str(z)
    sname = 'CRTStrip_' + md5('_'.join((xx, yy, zz))).hexdigest()[:8]

    if not sname in solids_store:
        s = ET.SubElement(solids, 'box', name=sname, lunit="cm", x=xx, y=yy, z=zz)
        solids_store[sname] = s
    else:
        s = solids_store[sname]

    name = 'CRTStrip_' + sequential_id()
    vname = 'volAuxDetSensitive' + name
    v = ET.SubElement(structure, 'volume', name=vname)
    ET.SubElement(v, 'materialref', ref='Polystyrene')
    ET.SubElement(v, 'solidref', ref=sname)

    return s, v


def strip_array(x, y, z=1.0, nx=16):
    '''Build an edge-to-edge array of scintillator strips.'''
    xx = str(x * nx)
    yy = str(y - 2 * PAD)
    zz = str(z)
    sname = 'CRTStripArray_' + md5('_'.join((xx, yy, zz))).hexdigest()[:8]

    if not sname in solids_store:
        s = ET.SubElement(solids, 'box', name=sname, lunit="cm", x=xx, y=yy, z=zz)
        solids_store[sname] = s
    else:
        s = solids_store[sname]

    strips = []
    for i in range(nx):
        strips.append(strip(x, y, z))

    name = 'CRTStripArray_' + sequential_id()
    vname = 'volAuxDet' + name
    v = ET.SubElement(structure, 'volume', name=vname)
    ET.SubElement(v, 'materialref', ref='Polystyrene')
    ET.SubElement(v, 'solidref', ref=sname)

    for i, (es, ev) in enumerate(strips):
        pv = ET.SubElement(v, 'physvol')
        ET.SubElement(pv, 'volumeref', ref=ev.attrib['name'])

        dx = 1.0 * (2*i - nx + 1) / 2 * x
        posname = 'pos' + ev.attrib['name']
        ET.SubElement(pv, 'position', name=posname,
                      unit="cm", x=str(dx), y='0', z='0')

    return s, v


def module(x, y, z=1.0, nx=16):
    '''Build a module: a strip array and an aluminum cover.'''
    # Module
    xx = str(x * nx + 2 * PAD)
    yy = str(y)
    zz = str(z + 2 * PAD)
    sname = 'CRTModule_' + md5('_'.join((xx, yy, zz))).hexdigest()[:8]

    if not sname in solids_store:
        s = ET.SubElement(solids, 'box', name=sname, lunit="cm", x=xx, y=yy, z=zz)
        solids_store[sname] = s
    else:
        s = solids_store[sname]

    # Strip array
    es, ev = strip_array(x, y, z, nx)

    # Cover (module - strip array)
    cover_sname = 'CRTModuleCover_' + md5('_'.join((xx, yy, zz))).hexdigest()[:8]

    if not cover_sname in solids_store:
        scover = ET.SubElement(solids, 'subtraction', name=cover_sname)
        ET.SubElement(scover, 'first', ref=sname)
        ET.SubElement(scover, 'second', ref=es.attrib['name'])
        solids_store[cover_sname] = scover
    else:
        scover = solids_store[cover_sname]

    # Volumes
    # Cover
    cover_name = 'CRTModuleCover_' + sequential_id()
    vcname = 'vol' + cover_name
    vc = ET.SubElement(structure, 'volume', name=vcname)
    ET.SubElement(vc, 'materialref', ref='ALUMINUM_Al')
    ET.SubElement(vc, 'solidref', ref=cover_sname)

    # Module
    name = 'CRTModule_' + sequential_id()
    vname = 'vol' + name
    v = ET.SubElement(structure, 'volume', name=vname)
    ET.SubElement(v, 'materialref', ref='ALUMINUM_Al')
    ET.SubElement(v, 'solidref', ref=sname)

    pvc = ET.SubElement(v, 'physvol')
    poscname = 'pos' + cover_name
    ET.SubElement(pvc, 'volumeref', ref=vcname)
    ET.SubElement(pvc, 'position', name=poscname, unit="cm", x='0', y='0', z='0')

    pvs = ET.SubElement(v, 'physvol')
    possname = 'pos' + ev.attrib['name']
    ET.SubElement(pvs, 'volumeref', ref=ev.attrib['name'])
    ET.SubElement(pvs, 'position', name=possname, unit="cm", x='0', y='0', z='0')

    return s, v


def tagger(name, x1, y1, nx1, ny1, x2, y2, nx2, ny2, z=1.0, nxs=16, back=False):
    '''Build a tagger: a stack of two perpendicular planes.

    The back face tagger has a 1.8m x 1.8m opening to provide clearance for
    cryogenics equipment. These deletions are enabled when back is True.
    '''
    # Tagger solid
    xx = '288.0' #str((x1 * nxs + 2 * PAD) * nx1)
    yy = '288.0' #str(y1 * ny1)
    zz = str(2.0 * (z + 2 * PAD))
    sname = 'Tagger' + name + '_' + md5('_'.join((xx, yy, zz))).hexdigest()[:8]

    if not sname in solids_store:
        s = ET.SubElement(solids, 'box', name=sname, lunit="cm", x=xx, y=yy, z=zz)
        solids_store[sname] = s
    else:
        s = solids_store[sname]

    # Solids for planes of modules
    planes = []
    pd = ((x1, y1, nx1, ny1), (x2, y2, nx2, ny2))
    xshort, yshort = 11.2, 180.0
    for k, plane in enumerate(pd):
        px, py, pnx, pny = plane

        modules = []
        for i in range(pnx):
            for j in range(pny):
                if back and ((k == 0 and i == 2 and j == 0) or
                             (k == 1 and i == 1 and j == 1)):
                    modules.append(module(xshort, yshort, z, nxs))
                else:
                    modules.append(module(px, py, z, nxs))
 
        planes.append(modules)

    # Tagger volume
    name = 'Tagger' + name
    vname = 'vol' + name
    v = ET.SubElement(structure, 'volume', name=vname)
    ET.SubElement(v, 'materialref', ref='ALUMINUM_Al')
    ET.SubElement(v, 'solidref', ref=sname)

    # Module volumes
    for k, (p, modules) in enumerate(zip(*(pd, planes))):
        px, py, pnx, pny = p
        for i in range(pnx):
            for j in range(pny):
                es, ev = modules[j + i*pny]

                pv = ET.SubElement(v, 'physvol')
                ET.SubElement(pv, 'volumeref', ref=ev.attrib['name'])

                dx = 1.0 * (2*i - pnx + 1) / 2 * (px * nxs + 2 * PAD)
                dy = 1.0 * (2*j - pny + 1) / 2 * py
                dz = (z + 2 * PAD) / 2

                # The short modules shift up by half a length
                if back and k == 0 and i == 2 and j == 0:
                    dy -= py / 4
                if back and k == 1 and i == 1 and j == 1:
                    dy += py / 4

                if k == 1:
                    dz *= -1
                    dx, dy = dy, dx

                posname = 'pos' + ev.attrib['name']
                ET.SubElement(pv, 'position', name=posname,
                              unit="cm", x=str(dx), y=str(dy), z=str(dz))

                if k == 1:
                    posname = 'rot' + ev.attrib['name']
                    ET.SubElement(pv, 'rotation', name=posname,
                                  unit="deg", x='0', y='0', z='90')

    return s, v


# Build the taggers
tag0s, tag0v = tagger('Array0', 5.95, 272.0, 3, 1, 5.95, 272.0, 3, 1)
tag0s, tag0v = tagger('Array1', 5.95, 272.0, 3, 1, 5.95, 272.0, 3, 1)
tag0s, tag0v = tagger('Array2', 5.95, 272.0, 3, 1, 5.95, 272.0, 3, 1)
tag0s, tag0v = tagger('Array3', 5.95, 272.0, 3, 1, 5.95, 272.0, 3, 1)

# Generate GDML for the world volume, for testing
#ws = ET.SubElement(solids, 'box', name='World', lunit="cm", x='10000', y='10000', z='10000')
#w = ET.SubElement(structure, 'volume', name='volWorld')
#ET.SubElement(w, 'materialref', ref='Air')
#ET.SubElement(w, 'solidref', ref='World')
#pv = ET.SubElement(w, 'physvol')
#ET.SubElement(pv, 'volumeref', ref=tag0v.attrib['name'])
#ET.SubElement(pv, 'position', name='posA', unit="cm", x='0', y='0', z='0')
#setup = ET.SubElement(gdml, 'setup', name='Default', version='1.0')
#ET.SubElement(setup, 'world', ref='volWorld')
#mats = ET.parse('mats.gdml')
#gdml.insert(0, mats.getroot())

with open('crt.gdml', 'w') as f:
    f.write(minidom.parseString(ET.tostring(gdml)).toprettyxml(indent=' '))

