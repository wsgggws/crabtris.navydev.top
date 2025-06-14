// 游戏常量
const BOARD_WIDTH = 22;
const BOARD_HEIGHT = 10;
const CELL_SIZE = 20;
const SHAPES = [
    // O形状
    {
        blocks: [
            {x: 0, y: 0}, {x: 1, y: 0},
            {x: 0, y: 1}, {x: 1, y: 1}
        ]
    },
    // J形状
    {
        blocks: [
            {x: 0, y: 0},
            {x: 0, y: 1}, {x: 1, y: 1}, {x: 2, y: 1}
        ]
    },
    // L形状
    {
        blocks: [
                              {x: 2, y: 0},
            {x: 0, y: 1}, {x: 1, y: 1}, {x: 2, y: 1}
        ]
    },
    // T形状
    {
        blocks: [
                 {x: 1, y: 0},
            {x: 0, y: 1}, {x: 1, y: 1}, {x: 2, y: 1}
        ]
    },
    // I形状
    {
        blocks: [
            {x: 0, y: 0}, {x: 1, y: 0}, {x: 2, y: 0}, {x: 3, y: 0}
        ]
    },
    // S形状
    {
        blocks: [
                 {x: 1, y: 0}, {x: 2, y: 0},
            {x: 0, y: 1}, {x: 1, y: 1}
        ]
    },
    // Z形状
    {
        blocks: [
            {x: 0, y: 0}, {x: 1, y: 0},
                          {x: 1, y: 1}, {x: 2, y: 1}
        ]
    }
];

// 游戏状态
let gameBoard = [];
let currentShape = null;
let currentX = 0;
let currentY = 0;
let score = 0;
let level = 1;
let isPaused = false;
let isGameOver = false;
let gameInterval = null;
let gameSpeed = 1000;

// 获取DOM元素
const canvas = document.getElementById('game-board');
const ctx = canvas.getContext('2d');
const scoreElement = document.getElementById('score');
const levelElement = document.getElementById('level');
const startButton = document.getElementById('start-btn');
const pauseButton = document.getElementById('pause-btn');
const rotateButton = document.getElementById('rotate-btn');
const upButton = document.getElementById('up-btn');
const rightButton = document.getElementById('right-btn');
const downButton = document.getElementById('down-btn');
const dropButton = document.getElementById('drop-btn');

// 初始化游戏
function initGame() {
    // 创建空游戏板
    gameBoard = Array(BOARD_HEIGHT).fill().map(() => Array(BOARD_WIDTH).fill(0));
    
    // 重置游戏状态
    score = 0;
    level = 1;
    gameSpeed = 1000;
    scoreElement.textContent = score;
    levelElement.textContent = level;
    isPaused = false;
    isGameOver = false;
    
    // 清空画布
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // 生成第一个方块
    generateNewShape();
    
    // 绘制游戏板
    drawBoard();
}

// 生成新的方块
function generateNewShape() {
    const shapeIndex = Math.floor(Math.random() * SHAPES.length);
    currentShape = JSON.parse(JSON.stringify(SHAPES[shapeIndex]));
    
    // 从左侧生成
    currentX = 0;
    // 垂直居中
    currentY = Math.floor((BOARD_HEIGHT - 2) / 2);
    
    // 检查游戏是否结束
    if (!isValidMove(currentX, currentY, currentShape)) {
        gameOver();
        return;
    }
    
    // 更新游戏板
    drawBoard();
}

// 绘制游戏板
function drawBoard() {
    // 清空画布
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // 绘制背景网格
    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, BOARD_WIDTH * CELL_SIZE, BOARD_HEIGHT * CELL_SIZE);
    
    // 绘制网格线
    ctx.strokeStyle = '#333';
    ctx.lineWidth = 0.5;
    
    for (let x = 0; x <= BOARD_WIDTH; x++) {
        ctx.beginPath();
        ctx.moveTo(x * CELL_SIZE, 0);
        ctx.lineTo(x * CELL_SIZE, BOARD_HEIGHT * CELL_SIZE);
        ctx.stroke();
    }
    
    for (let y = 0; y <= BOARD_HEIGHT; y++) {
        ctx.beginPath();
        ctx.moveTo(0, y * CELL_SIZE);
        ctx.lineTo(BOARD_WIDTH * CELL_SIZE, y * CELL_SIZE);
        ctx.stroke();
    }
    
    // 绘制固定的方块
    for (let y = 0; y < BOARD_HEIGHT; y++) {
        for (let x = 0; x < BOARD_WIDTH; x++) {
            if (gameBoard[y][x] === 1) {
                ctx.fillStyle = '#f44336';
                ctx.fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
                ctx.strokeStyle = '#fff';
                ctx.strokeRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            }
        }
    }
    
    // 绘制当前移动中的方块
    if (currentShape) {
        ctx.fillStyle = '#2196f3';
        currentShape.blocks.forEach(block => {
            const x = currentX + block.x;
            const y = currentY + block.y;
            ctx.fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            ctx.strokeStyle = '#fff';
            ctx.strokeRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        });
    }
}

// 检查移动是否有效
function isValidMove(newX, newY, shape) {
    return shape.blocks.every(block => {
        const x = newX + block.x;
        const y = newY + block.y;
        return (
            x >= 0 && x < BOARD_WIDTH &&
            y >= 0 && y < BOARD_HEIGHT &&
            (gameBoard[y][x] === 0)
        );
    });
}

// 旋转方块
function rotateShape() {
    if (isPaused || isGameOver || !currentShape) return;
    
    const newShape = {
        blocks: currentShape.blocks.map(block => {
            // 围绕原点旋转90度
            return {
                x: -block.y,
                y: block.x
            };
        })
    };
    
    // 找到旋转后的边界
    let minX = Number.MAX_VALUE;
    newShape.blocks.forEach(block => {
        minX = Math.min(minX, block.x);
    });
    
    // 调整位置使方块不会有负坐标
    newShape.blocks.forEach(block => {
        block.x -= minX;
    });
    
    if (isValidMove(currentX, currentY, newShape)) {
        currentShape = newShape;
        drawBoard();
    }
}

// 向右移动
function moveRight() {
    if (isPaused || isGameOver || !currentShape) return;
    
    if (isValidMove(currentX + 1, currentY, currentShape)) {
        currentX++;
        drawBoard();
    } else {
        // 不能再向右移动，固定方块
        lockShape();
        clearRows();
        generateNewShape();
    }
}

// 向上移动
function moveUp() {
    if (isPaused || isGameOver || !currentShape) return;
    
    if (isValidMove(currentX, currentY - 1, currentShape)) {
        currentY--;
        drawBoard();
    }
}

// 向下移动
function moveDown() {
    if (isPaused || isGameOver || !currentShape) return;
    
    if (isValidMove(currentX, currentY + 1, currentShape)) {
        currentY++;
        drawBoard();
    }
}

// 快速移动到最右
function dropToRight() {
    if (isPaused || isGameOver || !currentShape) return;
    
    let newX = currentX;
    while (isValidMove(newX + 1, currentY, currentShape)) {
        newX++;
    }
    
    if (newX !== currentX) {
        currentX = newX;
        drawBoard();
        
        // 立即固定方块
        lockShape();
        clearRows();
        generateNewShape();
    }
}

// 锁定方块
function lockShape() {
    currentShape.blocks.forEach(block => {
        const x = currentX + block.x;
        const y = currentY + block.y;
        if (x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT) {
            gameBoard[y][x] = 1;
        }
    });
}

// 清除完整的列
function clearRows() {
    let rowsCleared = 0;
    
    for (let x = BOARD_WIDTH - 1; x >= 0; x--) {
        let isColumnFull = true;
        
        for (let y = 0; y < BOARD_HEIGHT; y++) {
            if (gameBoard[y][x] !== 1) {
                isColumnFull = false;
                break;
            }
        }
        
        if (isColumnFull) {
            rowsCleared++;
            
            // 向右移动列
            for (let y = 0; y < BOARD_HEIGHT; y++) {
                for (let i = x; i > 0; i--) {
                    gameBoard[y][i] = gameBoard[y][i-1];
                }
                gameBoard[y][0] = 0;
            }
            
            // 由于列被移除，再次检查同一位置
            x++;
        }
    }
    
    // 更新分数
    if (rowsCleared > 0) {
        let points = 0;
        switch (rowsCleared) {
            case 1: points = 40 * level; break;
            case 2: points = 100 * level; break;
            case 3: points = 300 * level; break;
            case 4: points = 1200 * level; break;
        }
        
        score += points;
        scoreElement.textContent = score;
        
        // 升级
        const newLevel = Math.floor(score / 1000) + 1;
        if (newLevel > level) {
            level = newLevel;
            levelElement.textContent = level;
            
            // 增加游戏速度
            gameSpeed = Math.max(100, 1000 - (level - 1) * 100);
            if (gameInterval) {
                clearInterval(gameInterval);
                gameInterval = setInterval(moveRight, gameSpeed);
            }
        }
    }
}

// 处理键盘输入
function handleKeyDown(e) {
    if (isGameOver) return;
    
    switch(e.key) {
        case 'ArrowUp':
        case 'k':
        case 'K':
            moveUp();
            break;
        case 'ArrowDown':
        case 'j':
        case 'J':
            moveDown();
            break;
        case 'ArrowRight':
        case 'l':
        case 'L':
            moveRight();
            break;
        case ' ':
            dropToRight();
            break;
        case 'r':
        case 'R':
        case 'h':
        case 'H':
            rotateShape();
            break;
        case 'p':
        case 'P':
            togglePause();
            break;
    }
}

// 暂停/继续游戏
function togglePause() {
    if (isGameOver) return;
    
    isPaused = !isPaused;
    
    if (isPaused) {
        clearInterval(gameInterval);
        pauseButton.textContent = '继续';
        
        // 显示暂停信息
        ctx.fillStyle = 'rgba(0, 0, 0, 0.5)';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        ctx.font = '20px Arial';
        ctx.fillStyle = 'white';
        ctx.textAlign = 'center';
        ctx.fillText('游戏已暂停 - 按P继续', canvas.width / 2, canvas.height / 2);
    } else {
        gameInterval = setInterval(moveRight, gameSpeed);
        pauseButton.textContent = '暂停';
        drawBoard();
    }
}

// 游戏结束
function gameOver() {
    isGameOver = true;
    clearInterval(gameInterval);
    
    // 显示游戏结束信息
    ctx.fillStyle = 'rgba(0, 0, 0, 0.7)';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.font = '24px Arial';
    ctx.fillStyle = 'white';
    ctx.textAlign = 'center';
    ctx.fillText('游戏结束!', canvas.width / 2, canvas.height / 2 - 30);
    ctx.font = '18px Arial';
    ctx.fillText(`最终分数: ${score}  等级: ${level}`, canvas.width / 2, canvas.height / 2 + 10);
    
    startButton.textContent = '重新开始';
}

// 开始游戏
function startGame() {
    clearInterval(gameInterval);
    initGame();
    gameInterval = setInterval(moveRight, gameSpeed);
    startButton.textContent = '重新开始';
    pauseButton.textContent = '暂停';
}

// 设置事件监听器
document.addEventListener('keydown', handleKeyDown);
startButton.addEventListener('click', startGame);
pauseButton.addEventListener('click', togglePause);
rotateButton.addEventListener('click', rotateShape);
upButton.addEventListener('click', moveUp);
rightButton.addEventListener('click', moveRight);
downButton.addEventListener('click', moveDown);
dropButton.addEventListener('click', dropToRight);

// 初始化游戏
initGame();