# Rendering Granular Materials at the Mesoscopic Scale

MSc Thesis by Naglis Naslenas [PDF](/Rendering%20Granular%20Materials%20at%20the%20Mesoscopic%20Scale.pdf)

Project made in OpenGL and C++, equivalent scene was constructed in PBRT for comparison.


![](/images/grain_rotating.gif)


## Abstract

This master thesis explores the rendering of granular materials at a mesoscopic level
in real-time. The goal of the project is to develop a real-time rendering solution that
represents granular aggregate as individual particles, focusing on accurately represent-
ing the appearance of a single piece as well as making considerations for the medium
as a whole. The work explores initial simple diffuse reflection model to establish equal
conditions for the comparisons between the real-time and offline renderer, implemen-
tation of physically accurate specular reflection model and three methods to represent
individual grains are explored and implemented focusing on light interactions within
the grain object by leveraging the concept of Bidirectional Surface Scattering Distri-
bution Function (BSSRDF) to account for these complex light interactions. These
methods were then compared to the results from an offline renderer PBRT. Findings
from this project suggest that the three proposed methods for representing individual
grains in the real-time could be used to represent materials of different properties and
appearances.

## Example comparison between real-time and offline rendering

![](/images/final_grain1.png)