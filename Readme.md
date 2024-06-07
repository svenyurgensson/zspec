<a name="readme-top"></a>


<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]


<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/svenyurgensson/zspec">
    <img src="assets/logo.jpg" alt="Logo" width="120" height="120">
  </a>

<h3 align="center">ZSpec</h3>

  <p align="center">
    Z80 code tester and verificator
    <br />
    <a href="https://github.com/svenyurgensson/zspec/tree/main/Readme.md"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/svenyurgensson/zspec/tree/main/examples">View Examples</a>
    ·
    <a href="https://github.com/svenyurgensson/zspec/issues/new?labels=bug&template=bug-report---.md">Report Bug</a>
    ·
    <a href="https://github.com/svenyurgensson/zspec/issues/new?labels=enhancement&template=feature-request---.md">Request Feature</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About ZSpec

[![ZSpec Screen Shot][product-screenshot]](https://github.com/svenyurgensson/zspec)

Briefly – it is pretty pourly and carelessly written initial implementation tests for Z80 project's .

Perhaps, because of my long time work with high-level languages and web-projects I got a thought: why we doesn't have tests and specifications for Z80 projects? (May be they are exists, but I cannot found good one).

So I decided to make this one mainly with intention if someone wiser then me came and make something like this but much better.

<p align="right">(<a href="#readme-top">back to top</a>)</p>


### Built With

* [Z80 emulator library][z80-emul-url]
* [TomlPlusPlus parser library][tomlplusplus-url]
* [Toml][toml-url]
* C++ compiler


<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

* Write some library with functions using you lovely compiler: c or asm or something other
* Compile it, export binary image and labels file 
* Write specification, using simple language based on [Toml][toml-url] syntax
* Run `zspec` 
* You have results of calling your code in tradition `green`/`red` style

### Prerequisites

You need to have c++ compiler to build this project.


### Installation

Just `make` in project folder to build executable.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

WIP

_For more examples, please refer to the [Examples](https://github.com/svenyurgensson/zspec/tree/main/examples)_

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ROADMAP -->
## Roadmap

- [ ] Add ability to load binary file for expectations
- [ ] Add command line params
- [ ] More documentation and examples

See the [open issues](https://github.com/svenyurgensson/zspec/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Yury Batenko - jurbat@gmail.com

Project Link: [https://github.com/svenyurgensson/zspec](https://github.com/svenyurgensson/zspec)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->
## Acknowledgments


<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/svenyurgensson/zspec.svg?style=for-the-badge
[contributors-url]: https://github.com/svenyurgensson/zspec/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/svenyurgensson/zspec.svg?style=for-the-badge
[forks-url]: https://github.com/svenyurgensson/zspec/network/members
[stars-shield]: https://img.shields.io/github/stars/svenyurgensson/zspec.svg?style=for-the-badge
[stars-url]: https://github.com/svenyurgensson/zspec/stargazers
[issues-shield]: https://img.shields.io/github/issues/svenyurgensson/zspec.svg?style=for-the-badge
[issues-url]: https://github.com/svenyurgensson/zspec/issues
[license-shield]: https://img.shields.io/github/license/svenyurgensson/zspec.svg?style=for-the-badge
[license-url]: https://github.com/svenyurgensson/zspec/blob/master/LICENSE.txt
[product-screenshot]: assets/screenshot.png

[z80-emul-url]: https://github.com/suzukiplan/z80
[toml-url]: https://toml.io/en/
[tomlplusplus-url]: https://github.com/marzer/tomlplusplus