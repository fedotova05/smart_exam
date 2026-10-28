"""Обновляет список файлов в двух проектах Code::Blocks."""
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
for name in ['smart_exam', 'tests']:
    tree = ET.Element('CodeBlocks_project_file')
    ET.SubElement(tree, 'FileVersion', major='1', minor='6')
    project = ET.SubElement(tree, 'Project')
    ET.SubElement(project, 'Option', title='SmartExam' if name == 'smart_exam' else 'SmartExam tests')
    ET.SubElement(project, 'Option', compiler='gcc')
    build = ET.SubElement(project, 'Build')
    for mode in ['Debug', 'Release']:
        target = ET.SubElement(build, 'Target', title=mode)
        ET.SubElement(target, 'Option', output='../bin/' + mode + '/' + name, prefix_auto='1', extension_auto='1')
        ET.SubElement(target, 'Option', object_output='../obj/' + name + '/' + mode + '/')
        ET.SubElement(target, 'Option', type='1')
        ET.SubElement(target, 'Option', working_dir='..')
        compiler = ET.SubElement(target, 'Compiler')
        ET.SubElement(compiler, 'Add', option='-g' if mode == 'Debug' else '-O2')
    compiler = ET.SubElement(project, 'Compiler')
    for flag in ['-std=c++11', '-Wall', '-Wextra', '-D_WIN32_WINNT=0x0600', '-finput-charset=UTF-8', '-fexec-charset=UTF-8']:
        ET.SubElement(compiler, 'Add', option=flag)
    ET.SubElement(compiler, 'Add', directory='../src')
    ET.SubElement(compiler, 'Add', directory='../third_party/sqlite')
    files = sorted((root / 'src').rglob('*.h')) + sorted((root / 'src').rglob('*.cpp'))
    if name == 'smart_exam':
        ET.SubElement(compiler, 'Add', directory='$(#sfml)/include')
        linker = ET.SubElement(project, 'Linker')
        ET.SubElement(linker, 'Add', directory='$(#sfml)/lib')
        for library in ['sfml-graphics', 'sfml-window', 'sfml-system']:
            ET.SubElement(linker, 'Add', library=library)
        files += [root / 'main.cpp']
    else:
        files += sorted((root / 'tests').rglob('*.cpp')) + sorted((root / 'tests').rglob('*.h'))
    for file in files:
        ET.SubElement(project, 'Unit', filename='../' + file.relative_to(root).as_posix())
    unit = ET.SubElement(project, 'Unit', filename='../third_party/sqlite/sqlite3.c')
    ET.SubElement(unit, 'Option', compilerVar='CC')
    (root / 'project').mkdir(exist_ok=True)
    ET.indent(tree)
    ET.ElementTree(tree).write(root / 'project' / (name + '.cbp'), encoding='utf-8', xml_declaration=True)
