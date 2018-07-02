from .Experience import Experience
from .. import Mesh
from ..Simulation import Simulation
from ..Boundary import *
from ..Material import Material
from ..Behavior import Behavior
from ..Simulation import SofaSceneBuilder
from ..Utils import escape, memory_usage, bbox
from ..Report import HtmlReport
from ..PDE import *
from ..Optimization import *
from ..View import ParaView

import json
import os, sys
from math import pi as PI
import math
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


class CylinderExperience(Experience):
    def __init__(self, **kwargs):
        Experience.__init__(self, **kwargs)

        # Parameters
        self.radius = kwargs.get('radius', 7.5)
        self.length = kwargs.get('length', 80)
        self.pressure = kwargs.get('pressure', [0, -1e4, 0])
        self.surface_size = kwargs.get('surface_size')
        self.export_directory = kwargs.get('export_directory')

        # Members
        self.surface_mesh = Mesh.cylinder(center1=[0, 0, 0], center2=[0, 0, self.length], radius=self.radius,
                                          size=self.surface_size, dimension=2, quads=False)
        self.surface_mesh_filename = ""
        self.surface_img_filename = ""

    def serialize(self, keys=[], recursive=True):
        self.surface_mesh_filename = "An error has occurred while exporting the surface mesh"
        mesh_directory = self.export_directory
        if self.surface_mesh is not None and self.surface_mesh.vertices.size > 0:
            if mesh_directory is not None:
                if not os.path.isdir(mesh_directory):
                    os.mkdir(mesh_directory)
                    if not os.path.isdir(mesh_directory):
                        self.surface_mesh_filename = "Unabled to create output directory '{}'".format(mesh_directory)

                if os.path.isdir(mesh_directory):
                    # INITIAL SURFACE VTK EXPORT
                    filename = 'initial_surface_{}.vtk'.format(escape(self.name))
                    initial_vtk_filepath = os.path.join(mesh_directory, filename)
                    Mesh.toVtkFile(initial_vtk_filepath, self.surface_mesh)
                    self.surface_mesh_filename = filename

                    xmin, xmax, ymin, ymax, zmin, zmax = bbox(self.surface_mesh.vertices)
                    ini_width, ini_height, ini_length = xmax - xmin, ymax - ymin, zmax - zmin

                    for case in self.cases:
                        if case.solution is not None:
                            n = self.surface_mesh.vertices.shape[0]
                            if len(case.solution) == n:
                                mesh = Mesh.Mesh(
                                    vertices=np.array(case.solution),
                                    parts=self.surface_mesh.parts,
                                    gmsh=self.surface_mesh.gmsh
                                )

                                # CASE'S SOLUTION SURFACE VTK EXPORT
                                filename = 'solution_surface_{}.vtk'.format(escape(case.name))
                                case.solution_mesh_filename = filename
                                solution_mesh_filepath = os.path.join(mesh_directory, filename)
                                Mesh.toVtkFile(solution_mesh_filepath, mesh)

                                # CASE'S SOLUTION SURFACE IMAGE EXPORT
                                filename = 'solution_surface_{}.png'.format(escape(case.name))
                                case.solution_image_filename = filename
                                solution_image_filepath = os.path.join(mesh_directory, filename)
                                xmin, xmax, ymin, ymax, zmin, zmax = bbox(case.solution)
                                width, height, length = max(ini_width, xmax-xmin), max(ini_height, ymax-ymin), max(ini_length, zmax-zmin)

                                image_width = 1000
                                image_height = int(height / width * image_width)
                                camera_angle = 20
                                camera_x = xmax + (length / 2. / math.tan(math.radians(camera_angle/2.))) * 1.15
                                camera_y = ymin + (height / 2.)
                                camera_z = zmin + (length / 2.)

                                ParaView(
                                    size=(image_width, image_height),
                                    camera=ParaView.Camera(
                                        angle=camera_angle,
                                        position=[-camera_x, camera_y, camera_z],
                                        focal_point=[xmax, camera_y, camera_z],
                                    ),
                                    views=[
                                        ParaView.View(
                                            vtk_file=initial_vtk_filepath,
                                            line_width=0.01,
                                            color=[1, 0, 0],
                                            opacity=0.1
                                        ),
                                        ParaView.View(
                                            vtk_file=solution_mesh_filepath,
                                        )
                                ]).save(solution_image_filepath)

                            else:
                                print "SOLUTION NOT SAME SIZE ({} vs {})".format(len(case.solution), self.surface_mesh.vertices.size / 3.)
                        else:
                            print "NO SOLUTION FOUND"
        else:
            self.surface = "The surface is empty"

        keys = keys + ['radius', 'length', 'pressure', 'surface_size', 'surface_mesh']
        return Experience.serialize(self, keys)

    def create_report(self):
        pressure = np.linalg.norm(self.pressure)

        ntrian = self.surface_mesh.surface.triangles.shape[0]
        nquads = self.surface_mesh.surface.quads.shape[0]

        report = HtmlReport(name=self.name)
        report.add_section(name="Simulation")
        report.add_list(name="Parameters", attributes=[
            ('Radius', '{} {}'.format(self.radius, self.unit['length'])),
            ('Length', '{} {}'.format(self.length, self.unit['length'])),
            ('Volume', '{} {}<sup>3</sup>'.format(self.radius * self.radius * PI * self.length, self.unit['length'])),
            ('Density', '{} {}/{}<sup>3</sup>'.format(self.density, self.unit['mass'], self.unit['length'])),
            ('Mass', '{} {}'.format(self.radius * self.radius * PI * self.length * self.density, self.unit['mass'])),
            ('Pressure', '{} {}'.format(pressure, self.unit['pressure'])),
            ('Load surface', '{} {}<sup>2</sup>'.format(self.radius * self.radius * PI, self.unit['length'])),
            ('Load force', '{} {}'.format(self.radius * self.radius * PI * pressure / 1e6, self.unit['load'])),
            ('Young modulus', '{} {}'.format(self.young_modulus, self.unit['pressure'])),
            ('Poisson ratio', self.poisson_ratio),
            ('Number of steps', self.number_of_steps),
            ('Number of surface elements',
             ntrian + nquads),
        ])

        count = 0
        for case in self.cases:
            count = count + 1

            nnodes = case.behavior_mesh.vertices.shape[0]
            ntetra = case.behavior_mesh.volume.tetrahedrons.shape[0]
            nhexas = case.behavior_mesh.volume.hexahedrons.shape[0]

            report.add_section('Experience {} : {}'.format(count, case.name))
            report.add_image(path=case.solution_image_filename)
            report.add_list(name='Mesh', attributes=[
                ('Number of nodes', nnodes),
                ('Number of tetrahedrons', ntetra),
                ('Number of hexahedrons', nhexas),
            ])

            report.add_list('PDE Solver', [('Type', case.solver.fullname())] + case.solver.printable_attributes())

            system_solver = case.solver.solver
            if isinstance(system_solver, NonLinearSolver):
                report.add_list(
                    'Nonlinear solver',
                    [('Type', system_solver.fullname())] + system_solver.printable_attributes()
                )
                linear_solver = system_solver.linearSolver
            else:
                linear_solver = system_solver

            report.add_list(
                'Linear solver',
                [('Type', linear_solver.fullname())] + linear_solver.printable_attributes()
            )

            report.add_list(
                'Material',
                [('Type', case.material.fullname())] + case.material.printable_attributes()
            )

            report.add_list(
                'Behavior',
                [('Type', case.behavior.fullname())] + case.behavior.printable_attributes()
            )

            p = pressure * 1 / self.number_of_steps * self.radius * self.radius * PI
            steptimes = [0]
            newtonsteptimes = [0]
            pressures = [p]
            if len(case.steps) and len(case.steps[0].newtonsteps):
                forces = [p + case.steps[0].newtonsteps[0].residual]
            else:
                forces = [0]

            lasttime = 0

            for i in range(len(case.steps)):
                step = case.steps[i]
                if not len(step.newtonsteps):
                    break
                p = pressure * (i + 1) / self.number_of_steps * self.radius * self.radius * PI
                for newtonstep in step.newtonsteps:
                    lasttime = lasttime + newtonstep.duration
                    newtonsteptimes.append(lasttime)
                    forces.append(p + newtonstep.residual)
                steptimes.append(lasttime)
                pressures.append(p)

            if len(newtonsteptimes) < 2:
                report.add_paragraph(name='Convergence', text='Simulation has diverged')
                continue

            # Convergence graph
            plt.figure(figsize=(20, 10), dpi=300)

            df = pd.DataFrame({'time': newtonsteptimes, 'internal force': forces})
            plt.semilogy('time', 'internal force', data=df, color='skyblue', linewidth=1)

            dp = pd.DataFrame({'time': steptimes, 'external force': pressures})
            plt.step('time', 'external force', data=dp, marker='', color='red', linewidth=1, linestyle='dashed')

            for vline in steptimes:
                plt.axvline(x=vline, color='k', linestyle='--')

            plt.legend()
            img = os.path.realpath(
                os.path.join(self.export_directory, 'convergence_graph_{}.png'.format(escape(case.name))))
            plt.savefig(img, bbox_inches='tight')
            report.add_image(name="Convergence", path=img)
            print "Convergence exported at {}".format(img)

        report_path = os.path.join(self.export_directory, 'report_{}.html'.format(escape(self.name)))
        report.write(report_path)
        print "Report exported at '{}'".format(report_path)

    def run(self):
        sofa_simulation = self.sofa.createSimulation("DAG", self.name)
        self.sofa.setSimulation(sofa_simulation)
        self.sofa.timerSetEnabled(self.name, True)
        self.sofa.timerSetInterval(self.name, 1)
        self.sofa.timerSetOutputType(self.name, 'json')

        count = 0
        for case in self.cases:
            print "======= RUNNING CASE {} =======".format(case.name)
            count = count + 1
            behavior_mesh = Mesh.cylinder(
                center1=[0, 0, 0], center2=[0, 0, self.length], radius=self.radius, size=case.element_size,
                dimension=3, quads=False)

            case.behavior_mesh = behavior_mesh

            simulation = Simulation()
            simulation.add_meshes([
                self.surface_mesh,
                behavior_mesh
            ])
            simulation.set_PDE_solver(case.solver)

            boundaries = [
                FixedBoundary(part=behavior_mesh.base, linked_to=behavior_mesh.volume),
                PressureBoundary(part=behavior_mesh.top, pressure=self.pressure, slope=1. / self.number_of_steps,
                                 linked_to=behavior_mesh.volume),
            ]
            watcher = WatcherBoundary(part=self.surface_mesh.surface, linked_to=behavior_mesh.volume, link_type=case.link_type)
            simulation.add_boundaries(boundaries + [watcher])

            # Material setup
            mat_options = {
                'part': behavior_mesh.volume,
                'young_modulus': self.young_modulus,
                'poisson_ratio': self.poisson_ratio,
                'density': self.density,
            }

            if isinstance(case.material, tuple):
                m, options = case.material
                if isinstance(options, dict):
                    mat_options.update(options)
                case.material = m(**mat_options)
            elif issubclass(case.material, Material):
                case.material = case.material(**mat_options)

            simulation.add_materials([
                case.material
            ])

            # Behavior setup
            beh_options = {
                'part': behavior_mesh.volume,
            }

            if isinstance(case.behavior, tuple):
                b, options = case.behavior
                if isinstance(options, dict):
                    beh_options.update(options)
                case.behavior = b(**beh_options)
            elif issubclass(case.behavior, Behavior):
                case.behavior = case.behavior(**beh_options)

            simulation.add_behaviors([
                case.behavior
            ])

            # Launch the sofa's simulation
            print "Memory usage before scene creation : {} MB".format(memory_usage())
            root = self.sofa.createNode("root")
            SofaSceneBuilder(simulation=simulation, node=root)
            sofa_simulation.init(root)
            print "Memory usage after scene creation : {} MB".format(memory_usage())

            for i in range(self.number_of_steps):
                self.sofa.timerBegin(self.name)
                root.simulationStep(1)
                timer_output = '{' + str(self.sofa.timerEnd(self.name, root)) + '}'

                if timer_output not in ['{None}', '{}']:
                    j = json.loads(timer_output)
                    try:
                        step_timer_output = j[j.keys()[0]]['records'][self.name]
                        mechanical = step_timer_output['Simulation::animate']['AnimateVisitor']['Mechanical']
                    except KeyError as err:
                        print "[ERROR] No timing records in the simulation's output. (missing key '{}')"\
                            .format(err.message)
                        break
                    try:
                        newtonraphson = mechanical['NewtonRaphsonSolver::Solve']
                        try:
                            newton_nb_iterations = newtonraphson['nb_iterations']
                        except KeyError:
                            print "[ERROR] No newton iterations at step {}".format(i)
                            break

                        newton_steps = []
                        for ii in range(int(newton_nb_iterations)):
                            newton_step = mechanical['NewtonRaphsonSolver::Solve']['step_{}'.format(ii)]
                            newton_steps.append(Experience.NewtonStep(
                                duration=newton_step['end_time'] - newton_step['start_time'],
                                residual=newton_step['residual'],
                                correction=newton_step['correction']
                            ))
                        case.add_step(Experience.Step(
                            duration=mechanical['end_time'] - mechanical['start_time'],
                            newtonsteps=newton_steps
                        ))
                    except KeyError:
                        case.add_step(Experience.Step(
                            duration=mechanical['end_time'] - mechanical['start_time'],
                        ))

            case.solution = watcher.state.position

            # End the sofa's simulation
            print "Memory before unloading : {} MB".format(memory_usage())
            sofa_simulation.unload(root)
            print "Memory after unloading : {} MB".format(memory_usage())