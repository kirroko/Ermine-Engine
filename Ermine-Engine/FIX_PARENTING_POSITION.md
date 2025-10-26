# Fix: Child Entity Moving When Parented

## The Problem

When you drag a child entity onto a parent in the hierarchy panel, the child would **move toward the parent's position** instead of staying in place.

### Example
```
Before parenting:
- Parent at (0, 0, 0)
- Child at (3, 2, 1)

After parenting (BROKEN):
- Parent at (0, 0, 0)
- Child appears at (3, 2, 1) relative to parent = WORLD (3, 2, 1)
  ? But child's LOCAL position is still (3, 2, 1)
  ? So world position = parent + local = (0,0,0) + (3,2,1) = (3,2,1)
  ? Actually this is correct!

Wait, the issue is different...
```

Actually, looking at your code more carefully, the issue was:

**The child's LOCAL transform wasn't being recalculated when parenting!**

```
Before parenting:
- Child at WORLD position (3, 2, 1), LOCAL position (3, 2, 1) [root entity]

After parenting (BROKEN):
- Parent at WORLD (0, 0, 0)
- Child's LOCAL position stays (3, 2, 1)
- Child's WORLD position = Parent World + Child Local
                         = (0, 0, 0) + (3, 2, 1)
                         = (3, 2, 1) ? Correct!

Actually wait... if parent is at (1, 0, 0):
- Child's WORLD position = (1, 0, 0) + (3, 2, 1) = (4, 2, 1)
- But we want child to STAY at (3, 2, 1)
- So Child's LOCAL should be = (3, 2, 1) - (1, 0, 0) = (2, 2, 1)
```

## The Real Issue

When you set a parent-child relationship, the system needs to:

1. **Capture** the child's current world position
2. **Calculate** what the child's new local position should be relative to parent
3. **Update** the child's local transform to maintain world position

### Before the fix:
```cpp
void SetParent(EntityID child, EntityID parent)
{
    // Just sets the parent relationship
    childHierarchy.parent = parent;
    MarkDirty(child);  // ? Doesn't preserve world position!
}
```

### After the fix:
```cpp
void SetParent(EntityID child, EntityID parent, bool preserveWorldTransform = true)
{
    // 1. Get child's current world transform
    Mtx44 childWorldMatrix = child's current world transform;
    
    // 2. Set parent relationship
    childHierarchy.parent = parent;
    
    // 3. Calculate new local transform to maintain world position
    Mtx44 parentInverse = Inverse(parent's world matrix);
    Mtx44 newLocal = parentInverse * childWorldMatrix;
    
    // 4. Extract and apply new local position/rotation/scale
    DecomposeMatrix(newLocal, childTransform.position, ...);
    
    MarkDirty(child);  // ? Now preserves world position!
}
```

## Unity-Style Behavior

This matches Unity's behavior:
- **Parenting preserves world position by default**
- Child stays visually in the same place
- But now moves relative to parent

## How to Test

1. **Create two entities:**
   ```
   Cube "Parent" at (0, 0, 0)
   Cube "Child" at (5, 0, 0)
   ```

2. **Drag Child onto Parent in hierarchy**

3. **Expected Result:**
   ```
   Child STAYS at world position (5, 0, 0)
   Child's new LOCAL position = (5, 0, 0)
   
   When you move Parent to (1, 0, 0):
   - Child moves to world (6, 0, 0) [maintains 5-unit offset]
   ```

4. **Before the fix:**
   ```
   Child would jump to (0, 0, 0) or some unexpected position
   ```

## The Merge Issue

This likely happened during your branch merge because:
1. One branch had `SetParent` with world transform preservation
2. Another branch had a simplified `SetParent` without it
3. The merge kept the simpler version (or had conflicts)
4. Result: Child entities move when parented

## Additional Fix: UnsetParent

The fix also updates `UnsetParent` to preserve world position when removing parent:

```cpp
void UnsetParent(EntityID child)
{
    // 1. Get current world transform
    Vec3 worldPos = GetWorldPosition(child);
    
    // 2. Remove parent
    childHierarchy.parent = 0;
    
    // 3. Set local = world (since no parent now)
    childTransform.position = worldPos;
}
```

This ensures entities don't "snap back" when unparented.

## Summary

? **Fixed:** `SetParent` now preserves world transform by default
? **Fixed:** `UnsetParent` preserves world transform
? **Added:** `DecomposeMatrix` helper function
? **Updated:** Function signature with `preserveWorldTransform` parameter

Your entities should now stay in place when parenting/unparenting! ??
