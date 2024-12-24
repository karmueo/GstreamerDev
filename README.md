# Vscode调试

## 目录

- [教程](#教程)
- [使用方法](#使用方法)

## 教程 <a name = "教程"></a>

### 1. 需要 VSCode 的 .devcontainer 文件来了解如何将 docker 容器挂载为工作区。参考`.devcontainer/devcontainer.json`

- devcontainer.json 文件是一个配置文件，主要用于 Visual Studio Code 的 Remote - Containers 扩展功能。这个文件定义了如何在容器中设置和管理开发环境，使得开发者能够使用完全定义和容器化的开发环境进行工作。它允许团队成员在统一且配置一致的开发环境中工作，减少了“在我这里可以运行”的问题。

### 2. 配置 task

- 设置task，以便可以构建和测试代码。参考`.vscode/tasks.json`

### 3. 配置调试

- 一旦可以构建代码，可能想要运行和调试它。可以通过向 `.vscode/launch.json` 文件添加不同的配置来完成此操作

## 使用方法 <a name = "使用方法"></a>

### 1. 先决条件

- [docker](https://docs.docker.com/engine/install/)
- [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

### 2. 用法

- 在Vscode中打开项目，按`ctrl+shift+p`，输入`Dev Containers: Reopen in Container`
