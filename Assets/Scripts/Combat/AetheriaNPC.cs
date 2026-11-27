using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria
{
    /// <summary>Named story NPC: idles, turns to the hero when approached, and floats a quest marker.</summary>
    public class AetheriaNPC : MonoBehaviour
    {
        public static readonly List<AetheriaNPC> AllNPCs = new List<AetheriaNPC>();
        public string NpcId;
        public string DisplayName;
        public string RoleTitle;
        public string QuestId;

        public float TurnRange = 7f;
        public float IdleAnimSpeed = 1f;

        private Transform _marker;
        private MeshRenderer _markerRenderer;
        private Animator _anim;
        private Transform _spine;
        private Quaternion _restSpineRot = Quaternion.identity;
        private Vector3 _baseScale = Vector3.one;
        private float _markerPhase;
        private float _breathPhase;
        private Color _markerColor;
        private bool _markerShown;

        private void OnEnable()
        {
            if (!AllNPCs.Contains(this)) AllNPCs.Add(this);
            FxAssets.Ensure();
            _baseScale = transform.localScale;
            _breathPhase = Random.value * 6.283f;

            _anim = GetComponentInChildren<Animator>(true);
            if (_anim != null)
            {
                if (_anim.runtimeAnimatorController == null) _anim = null;
                else
                {
                    _anim.applyRootMotion = false;
                    _anim.cullingMode = AnimatorCullingMode.AlwaysAnimate;
                    _anim.SetFloat("Speed", 0f);
                    _anim.speed = IdleAnimSpeed;
                }
            }
            _spine = AetheriaPalette.FindBoneAny(transform, "spine_02", "spine_01", "Chest");
            if (_spine != null) _restSpineRot = _spine.localRotation;

            if (_marker == null)
            {
                var go = new GameObject("QuestMarker");
                go.transform.SetParent(transform, false);
                go.AddComponent<MeshFilter>().sharedMesh = MarkerQuad();
                _markerRenderer = go.AddComponent<MeshRenderer>();
                _markerRenderer.shadowCastingMode = ShadowCastingMode.Off;
                _markerRenderer.receiveShadows = false;
                _markerRenderer.lightProbeUsage = LightProbeUsage.Off;
                _markerRenderer.reflectionProbeUsage = ReflectionProbeUsage.Off;
                _markerRenderer.sortingOrder = 20;
                _marker = go.transform;
                _markerShown = true;
            }
        }

        private static Mesh _quad;
        private static Mesh MarkerQuad()
        {
            if (_quad != null) return _quad;
            _quad = new Mesh { name = "QuestMarkerQuad" };
            _quad.SetVertices(new[] { new Vector3(-0.5f, -0.5f, 0f), new Vector3(0.5f, -0.5f, 0f), new Vector3(-0.5f, 0.5f, 0f), new Vector3(0.5f, 0.5f, 0f) });
            _quad.SetUVs(0, new[] { Vector2.zero, Vector2.right, Vector2.up, Vector2.one });
            _quad.SetTriangles(new[] { 0, 2, 1, 2, 3, 1 }, 0);
            _quad.RecalculateBounds();
            return _quad;
        }

        private void OnDisable() => AllNPCs.Remove(this);

        private void Update()
        {
            float dt = Time.deltaTime;
            _breathPhase += dt;
            var hero = HeroController.Instance;
            bool near = hero != null && Vector3.Distance(transform.position, hero.transform.position) < TurnRange;
            float headHeight = transform.lossyScale.y > 0.01f ? 2.25f / transform.lossyScale.y : 2.25f;

            // Quest marker: gold while there is something to do here, emerald when a reward is waiting
            var q = QuestAndMapService.Instance?.FindQuestForGiver(NpcId);
            bool show = q != null && q.State != QuestState.Completed && !string.IsNullOrEmpty(NpcId);
            Color want = q != null && q.State == QuestState.ReadyToTurnIn ? AetheriaPalette.TextEmerald : AetheriaPalette.TextGold;
            if (show != _markerShown || (show && want != _markerColor))
            {
                _markerShown = show;
                _markerColor = want;
                _marker.gameObject.SetActive(show);
                if (show) _markerRenderer.sharedMaterial = AetheriaRender.TransparentGlow(want, true, FxAssets.Star);
            }
            if (show)
            {
                _markerPhase += dt;
                float bob = 0.16f / Mathf.Max(0.2f, transform.lossyScale.y);
                _marker.localPosition = new Vector3(0f, headHeight + Mathf.Sin(_markerPhase * 2.4f) * bob, 0f);
                _marker.localScale = Vector3.one * (0.72f / Mathf.Max(0.15f, transform.lossyScale.y));
                var cam = Camera.main;
                if (cam != null) _marker.rotation = Quaternion.LookRotation(_marker.position - cam.transform.position, Vector3.up);
            }

            // Turn toward the hero
            if (near)
            {
                Vector3 toHero = hero.transform.position - transform.position;
                toHero.y = 0f;
                if (toHero.sqrMagnitude > 0.05f)
                    transform.rotation = Quaternion.RotateTowards(transform.rotation, Quaternion.LookRotation(toHero, Vector3.up), 220f * dt);
            }
            if (_anim != null)
            {
                _anim.SetFloat("Speed", 0f);
            }
            else
            {
                float s = Mathf.Sin(_breathPhase * 1.8f);
                if (_spine != null)
                    _spine.localRotation = _restSpineRot * Quaternion.Euler(s * 1.6f, 0f, 0f);
                else
                    transform.localScale = new Vector3(_baseScale.x, _baseScale.y * (1f + s * 0.012f), _baseScale.z);
            }
        }

        private void OnDestroy()
        {
            if (_markerRenderer != null) _markerRenderer.sharedMaterial = null;
        }
    }
}
